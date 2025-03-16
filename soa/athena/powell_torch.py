# /******************************************
# MIT License

# Copyright (c) 2025 Giuseppe Sorrentino, Paolo Salvatore Galfano, Davide Conficconi, Eleonora D'Arnese

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# *MIT License
# *
# *Copyright (c) [2021] [Eleonora D'Arnese, Emanuele Del Sozzo, Davide Conficconi,  Marco Domenico Santambrogio]
# *
# *Permission is hereby granted, free of charge, to any person obtaining a copy
# *of this software and associated documentation files (the "Software"), to deal
# *in the Software without restriction, including without limitation the rights
# *to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# *copies of the Software, and to permit persons to whom the Software is
# *furnished to do so, subject to the following conditions:
# *
# *The above copyright notice and this permission notice shall be included in all
# *copies or substantial portions of the Software.
# *
# *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# *SOFTWARE.
# ******************************************/

import os
import re
import pydicom
import cv2
import numpy as np
import math
import glob
import time
import pandas as pd
from torch.multiprocessing import Pool, Process, set_start_method
import struct
import statistics
import argparse
import kornia
import torch

compute_metric = None
precompute_metric = None
device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
ref_vals = None
move_data = None
def no_transfer(input_data):
    return input_data

def to_cuda(input_data):
    return input_data.cuda(non_blocking=True)

def batch_transform(images, pars, volume):
    img_warped = kornia.geometry.warp_affine3d(images, pars, dsize=(volume, images.shape[3], images.shape[4]), align_corners = True)
    return img_warped

def transform(image, par, volume):
    tmp_img = image.reshape((1, 1, *image.shape)).float()
    t_par = torch.unsqueeze(par, dim=0)
    img_warped = kornia.geometry.warp_affine3d(tmp_img, t_par, dsize=(volume, tmp_img.shape[3], tmp_img.shape[4]), align_corners = True)
    return img_warped

def compute_moments(img):
    moments = torch.empty(6, device=device)
    l = torch.arange(img.shape[0], device=device)
    moments[0] = torch.sum(img) # m00
    moments[1] = torch.sum(img * l) # m10
    moments[2] = torch.sum(img * (l**2)) # m20
    moments[3] = torch.sum(img * l.reshape((img.shape[0], 1)) ) # m01
    moments[4] = torch.sum(img * (l.reshape((img.shape[0], 1)))**2 ) # m02
    moments[5] = torch.sum(img * l * l.reshape((img.shape[0], 1))) # m11
    return moments

def to_matrix_complete(vector_params):
    """
        vector_params contains tx, ty, tz for translation on x, y and z axes respectively
        and cosine of phi, theta, psi for rotations around x, y, and z axes respectively.
    """
    mat_params=torch.empty((3,4))
    mat_params[0][3]=vector_params[0] 
    mat_params[1][3]=vector_params[1] 
    mat_params[2][3]=vector_params[2]
    cos_phi = vector_params[3]
    cos_theta = vector_params[4]
    cos_psi = vector_params[5]
    if cos_phi > 1 or cos_phi < -1:
        cos_phi = 1
    if cos_theta > 1 or cos_theta < -1:
        cos_theta = 1
    if cos_psi > 1 or cos_psi < -1:
        cos_psi = 1
    sin_phi = -np.sqrt(1-(cos_phi**2))
    sin_theta = -np.sqrt(1-(cos_theta**2))
    sin_psi = -np.sqrt(1-(cos_psi**2))
    sin_theta_sin_psi = sin_theta * sin_psi
    sin_theta_cos_psi = sin_theta * cos_psi
    cos_theta_cos_psi = cos_theta * cos_psi
    cos_theta_sin_psi = cos_theta * sin_psi
    cos_phi_cos_psi = cos_phi * cos_psi
    cos_phi_sin_psi = cos_phi * sin_psi
    sin_phi_cos_theta = sin_phi * cos_theta
    sin_phi_sin_psi = sin_phi * sin_psi
    cos_phi_cos_theta = cos_phi * cos_theta
    sin_phi_cos_psi = sin_phi * cos_psi
    mat_params[0][0] = cos_theta_cos_psi
    mat_params[1][0] = cos_theta_sin_psi
    mat_params[2][0] = -sin_theta
    mat_params[0][1] = -cos_phi_sin_psi + sin_phi * sin_theta_cos_psi
    mat_params[1][1] = cos_phi_cos_psi + sin_phi * sin_theta_sin_psi
    mat_params[2][1] = sin_phi_cos_theta
    mat_params[0][2] = sin_phi_sin_psi + cos_phi * sin_theta_cos_psi
    mat_params[1][2] = -sin_phi_cos_psi + cos_phi * sin_theta_sin_psi
    mat_params[2][2] = cos_phi_cos_theta
    return (mat_params)

def estimate_rho(Ref_uint8s,Flt_uint8s, params, volume):
    tot_flt_avg_10 = 0
    tot_flt_avg_01 = 0
    tot_flt_mu_20 = 0
    tot_flt_mu_02 = 0
    tot_flt_mu_11 = 0
    tot_ref_avg_10 = 0
    tot_ref_avg_01 = 0
    tot_ref_mu_20 = 0
    tot_ref_mu_02 = 0
    tot_ref_mu_11 = 0
    tot_params1 = 0
    tot_params2 = 0
    tot_roundness = 0
    for i in range(0, volume):
        Ref_uint8 = Ref_uint8s[i, :, :]
        Flt_uint8 = Flt_uint8s[i, :, :]
        
        try:
            ref_mom = compute_moments(Ref_uint8)
            #print(ref_mom)
            flt_mom = compute_moments(Flt_uint8)
            #print(flt_mom)

            flt_avg_10 = flt_mom[1]/flt_mom[0]
            flt_avg_01 = flt_mom[3]/flt_mom[0]
            flt_mu_20 = (flt_mom[2]/flt_mom[0]*1.0)-(flt_avg_10*flt_avg_10)
            flt_mu_02 = (flt_mom[4]/flt_mom[0]*1.0)-(flt_avg_01*flt_avg_01)
            flt_mu_11 = (flt_mom[5]/flt_mom[0]*1.0)-(flt_avg_01*flt_avg_10)
            ref_avg_10 = ref_mom[1]/ref_mom[0]
            ref_avg_01 = ref_mom[3]/ref_mom[0]
            ref_mu_20 = (ref_mom[2]/ref_mom[0]*1.0)-(ref_avg_10*ref_avg_10)
            ref_mu_02 = (ref_mom[4]/ref_mom[0]*1.0)-(ref_avg_01*ref_avg_01)
            ref_mu_11 = (ref_mom[5]/ref_mom[0]*1.0)-(ref_avg_01*ref_avg_10)
            
            params[0] = ref_mom[1]/ref_mom[0] - flt_mom[1]/flt_mom[0]
            params[1] = ref_mom[3]/ref_mom[0] - flt_mom[3]/flt_mom[0]

            roundness=(flt_mom[2]/flt_mom[0]) / (flt_mom[4]/flt_mom[0])
            tot_flt_avg_10 += flt_avg_10
            tot_flt_avg_01 += flt_avg_01
            tot_flt_mu_20 += flt_mu_20
            tot_flt_mu_02 += flt_mu_02
            tot_flt_mu_11 += flt_mu_11
            tot_ref_avg_10 += ref_avg_10
            tot_ref_avg_01 += ref_avg_01
            tot_ref_mu_20 += ref_mu_20
            tot_ref_mu_02 += ref_mu_02
            tot_ref_mu_11 += ref_mu_11
            tot_params1 += params[0]
            tot_params2 += params[1]
            tot_roundness += roundness
            #print(i)
        except:
             continue
        
    tot_flt_avg_10 = tot_flt_avg_10/volume
    tot_flt_avg_01 = tot_flt_avg_01/volume
    tot_flt_mu_20 = tot_flt_mu_20/volume
    tot_flt_mu_02 = tot_flt_mu_02/volume
    tot_flt_mu_11 = tot_flt_mu_11/volume
    tot_ref_avg_10 = tot_ref_avg_10/volume
    tot_ref_avg_01 = tot_ref_avg_01/volume
    tot_ref_mu_20 = tot_ref_mu_20/volume
    tot_ref_mu_02 = tot_ref_mu_02/volume
    tot_ref_mu_11 = tot_ref_mu_11/volume
    tot_params1 = tot_params1/volume
    tot_params2 = tot_params2/volume
    tot_roundness = tot_roundness/volume
    try: 
        rho_flt=0.5*torch.atan((2.0*flt_mu_11)/(flt_mu_20-flt_mu_02))
        rho_ref=0.5*torch.atan((2.0*ref_mu_11)/(ref_mu_20-ref_mu_02))
        delta_rho=rho_ref-rho_flt
        #since the matrix we want to create is an affine matrix, the initial parameters have been prepared as a "particular" affine, the similarity matrix.
        if math.fabs(tot_roundness-1.0)<0.3:
            delta_rho = 0
    except Exception as e:
        delta_rho = 0
    
    #print(tot_params1, tot_params2)
    return torch.Tensor([delta_rho]), torch.Tensor([tot_params1]), torch.Tensor([tot_params2])

def estimate_initial3D(Ref_uint8s,Flt_uint8s, params, volume):
    params = torch.zeros((6,))
    psi, tx, ty = estimate_rho(Ref_uint8s, Flt_uint8s, params, volume)
    rot_ref_phi = torch.rot90(Ref_uint8s, dims=[0,2])
    rot_flt_phi = torch.rot90(Flt_uint8s, dims=[0,2])
    phi, _, _  = estimate_rho(rot_ref_phi, rot_flt_phi, params, volume)
    rot_ref_theta = torch.rot90(Ref_uint8s, dims=[1,2])
    rot_flt_theta = torch.rot90(Flt_uint8s, dims=[1,2])
    theta, _, _ = estimate_rho(rot_ref_theta, rot_flt_theta, params, volume)
    params = [tx, ty, 0, torch.cos(phi), torch.cos(theta), torch.cos(psi)]
    return params

def estimate_initial(Ref_uint8s,Flt_uint8s, params, volume):
    
    tot_flt_avg_10 = 0
    tot_flt_avg_01 = 0
    tot_flt_mu_20 = 0
    tot_flt_mu_02 = 0
    tot_flt_mu_11 = 0
    tot_ref_avg_10 = 0
    tot_ref_avg_01 = 0
    tot_ref_mu_20 = 0
    tot_ref_mu_02 = 0
    tot_ref_mu_11 = 0
    tot_params1 = 0
    tot_params2 = 0
    tot_roundness = 0
    for i in range(0, volume):
        Ref_uint8 = Ref_uint8s[i, :, :]
        Flt_uint8 = Flt_uint8s[i, :, :]
        try:
            ref_mom = compute_moments(Ref_uint8)
            flt_mom = compute_moments(Flt_uint8)
        except:
             continue
        flt_avg_10 = flt_mom[1]/flt_mom[0]
        flt_avg_01 = flt_mom[3]/flt_mom[0]
        flt_mu_20 = (flt_mom[2]/flt_mom[0]*1.0)-(flt_avg_10*flt_avg_10)
        flt_mu_02 = (flt_mom[4]/flt_mom[0]*1.0)-(flt_avg_01*flt_avg_01)
        flt_mu_11 = (flt_mom[5]/flt_mom[0]*1.0)-(flt_avg_01*flt_avg_10)
        ref_avg_10 = ref_mom[1]/ref_mom[0]
        ref_avg_01 = ref_mom[3]/ref_mom[0]
        ref_mu_20 = (ref_mom[2]/ref_mom[0]*1.0)-(ref_avg_10*ref_avg_10)
        ref_mu_02 = (ref_mom[4]/ref_mom[0]*1.0)-(ref_avg_01*ref_avg_01)
        ref_mu_11 = (ref_mom[5]/ref_mom[0]*1.0)-(ref_avg_01*ref_avg_10)
        
        params[0] = ref_mom[1]/ref_mom[0] - flt_mom[1]/flt_mom[0]
        params[1] = ref_mom[3]/ref_mom[0] - flt_mom[3]/flt_mom[0]

        roundness=(flt_mom[2]/flt_mom[0]) / (flt_mom[4]/flt_mom[0])
        tot_flt_avg_10 += flt_avg_10
        tot_flt_avg_01 += flt_avg_01
        tot_flt_mu_20 += flt_mu_20
        tot_flt_mu_02 += flt_mu_02
        tot_flt_mu_11 += flt_mu_11
        tot_ref_avg_10 += ref_avg_10
        tot_ref_avg_01 += ref_avg_01
        tot_ref_mu_20 += ref_mu_20
        tot_ref_mu_02 += ref_mu_02
        tot_ref_mu_11 += ref_mu_11
        tot_params1 += params[0]
        tot_params2 += params[1]
        tot_roundness += roundness
    tot_flt_avg_10 = tot_flt_avg_10/volume
    tot_flt_avg_01 = tot_flt_avg_01/volume
    tot_flt_mu_20 = tot_flt_mu_20/volume
    tot_flt_mu_02 = tot_flt_mu_02/volume
    tot_flt_mu_11 = tot_flt_mu_11/volume
    tot_ref_avg_10 = tot_ref_avg_10/volume
    tot_ref_avg_01 = tot_ref_avg_01/volume
    tot_ref_mu_20 = tot_ref_mu_20/volume
    tot_ref_mu_02 = tot_ref_mu_02/volume
    tot_ref_mu_11 = tot_ref_mu_11/volume
    tot_params1 = tot_params1/volume
    tot_params2 = tot_params2/volume
    tot_roundness = tot_roundness/volume
    try:
        rho_flt=0.5*torch.atan((2.0*flt_mu_11)/(flt_mu_20-flt_mu_02))
        rho_ref=0.5*torch.atan((2.0*ref_mu_11)/(ref_mu_20-ref_mu_02))
        delta_rho=rho_ref-rho_flt
        if math.fabs(tot_roundness-1.0)<0.3:
            delta_rho = torch.Tensor([0])
    except Exception:
        delta_rho = torch.Tensor([0])
    
    return torch.Tensor([tot_params1, tot_params2, 0, 1, 1, torch.cos(delta_rho)])

def my_squared_hist2d_t(sample, bins, smin, smax):
    D, N = sample.shape
    edges = torch.linspace(smin, smax, bins + 1, device=device)
    nbin = edges.shape[0] + 1
    
    # Compute the bin number each sample falls into.
    Ncount = D*[None]
    for i in range(D):
        Ncount[i] = torch.searchsorted(edges, sample[i, :], right=True)
    
    # Using digitize, values that fall on an edge are put in the right bin.
    # For the rightmost bin, we want values equal to the right edge to be
    # counted in the last bin, and not as an outlier.
    for i in range(D):
        # Find which points are on the rightmost edge.
        on_edge = (sample[i, :] == edges[-1])
        # Shift these points one bin to the left.
        Ncount[i][on_edge] -= 1
    
    # Compute the sample indices in the flattened histogram matrix.
    xy = Ncount[0]*nbin+Ncount[1]
           

    # Compute the number of repetitions in xy and assign it to the
    hist = torch.bincount(xy, None, minlength=nbin*nbin)
    
    # Shape into a proper matrix
    hist = hist.reshape((nbin, nbin))

    hist = hist.float()
    
    # Remove outliers (indices 0 and -1 for each dimension).
    hist = hist[1:-1,1:-1]
    
    return hist

def precompute_mutual_information(Ref_uint8_ravel):
    
    href = torch.histc(Ref_uint8_ravel, bins=256)
    href /= Ref_uint8_ravel.numel()
    href=href[href>0.000000000000001]
    eref=(torch.sum(href*(torch.log2(href))))*-1
    
    return eref

def mutual_information(Ref_uint8_ravel, Flt_uint8_ravel, eref):
    
    if(device == "cuda:0"):
        idx_joint = torch.stack((Ref_uint8_ravel, Flt_uint8_ravel)).long()
        j_h_init = torch.sparse.IntTensor(idx_joint, ref_vals, torch.Size([hist_dim, hist_dim])).to_dense()/Ref_uint8_ravel.numel()
    else:
        idx_joint = torch.stack((Ref_uint8_ravel, Flt_uint8_ravel))
        j_h_init = my_squared_hist2d_t(idx_joint, hist_dim, 0, 255)/Ref_uint8_ravel.numel()
    
    j_h = j_h_init[j_h_init>0.000000000000001]
    entropy=(torch.sum(j_h*(torch.log2(j_h))))*-1
    hflt=torch.sum(j_h_init,axis=0) 
    hflt=hflt[hflt>0.000000000000001]
    eflt=(torch.sum(hflt*(torch.log2(hflt))))*-1  
    mutualinfo=eref+eflt-entropy  
    return(mutualinfo)

def compute_mi(ref_img, flt_imgs, t_mats, eref, volume):
    flt_warped = batch_transform(flt_imgs, t_mats, volume)
    mi_a = mutual_information(ref_img, flt_warped[0].ravel(), eref)
    mi_b = mutual_information(ref_img, flt_warped[1].ravel(), eref)
    return torch.exp(-mi_a).cpu(), torch.exp(-mi_b).cpu()

counter = 0

def optimize_goldsearch(par, rng, ref_sup_ravel, flt_stack, linear_par, i, eref, volume):
    global compute_metric
    start=par-0.382*rng
    end=par+0.618*rng
    c=(end-(end-start)/1.618)
    d=(start+(end-start)/1.618)
    best_mi = 0.0
    while(math.fabs(c-d)>0.005):
        linear_par[i]=c
        a_mat=to_matrix_complete(linear_par)
        linear_par[i]=d
        b_mat=to_matrix_complete(linear_par)
        mats = move_data(torch.stack((a_mat, b_mat)))
        mi_a, mi_b = compute_mi(ref_sup_ravel, flt_stack, mats, eref, volume)
        global counter
        counter +=2
        if(mi_a < mi_b):
            end=d
            best_mi = mi_a
            linear_par[i]=c
        else:
            start=c
            best_mi = mi_b
            linear_par[i]=d
        c=(end-(end-start)/1.618)
        d=(start+(end-start)/1.618)
    return (end+start)/2, best_mi

def optimize_powell(rng, par_lin, ref_sup_ravel, flt_stack, eref, volume):
    converged = False
    eps = 0.000005
    last_mut=100000.0
    it=0
    while(not converged):
        converged=True
        it=it+1
        for i in range(par_lin.numel()):
            cur_par = par_lin[i]
            cur_rng = rng[i]
            param_opt, cur_mi = optimize_goldsearch(cur_par, cur_rng, ref_sup_ravel, flt_stack, par_lin, i, eref, volume)
            par_lin[i]=cur_par
            if last_mut-cur_mi>eps:
                par_lin[i]=param_opt
                last_mut=cur_mi
                converged=False
            else:
                par_lin[i]=cur_par
    return (par_lin)

def register_images(filename, Ref_uint8, Flt_uint8, volume):
    global precompute_metric
    start_single_sw = time.time()
    params = torch.empty((6,), device = device)
    params1 = estimate_initial(Ref_uint8, Flt_uint8, params, volume)
    for i in range(len(params1)):
        params[i] = params1[i]
    params_cpu = params.cpu()
    rng=torch.tensor([80.0, 80.0, 0.0, 1.0, 1.0, 1.0])
    pa = params_cpu.clone()
    Ref_uint8_ravel = Ref_uint8.ravel().double()
    eref = precompute_mutual_information(Ref_uint8_ravel)
    flt_u = torch.unsqueeze(Flt_uint8, dim=0).float()
    flt_stack = torch.stack((flt_u, flt_u))
    optimal_params = optimize_powell(rng, pa, Ref_uint8_ravel, flt_stack, eref,volume) 
    params_trans=to_matrix_complete(optimal_params)
    flt_transform = transform(Flt_uint8,move_data(params_trans), volume)
    end_single_sw = time.time()
    global counter
    print(f"Counter Value: {counter}")
    with open(filename, 'a') as file2:
                file2.write("%s\n" % (end_single_sw - start_single_sw))
 
    return (flt_transform)

def save_data(OUT_STAK, name, res_path, volume):
    OUT_STAK = torch.reshape(OUT_STAK,(volume,1,512, 512))
    for i in range(len(OUT_STAK)):
        b=name[i].split('/')
        c=b.pop()
        d=c.split('.')
        cv2.imwrite(os.path.join(res_path, d[0][0:2]+str(int(d[0][2:5])+1)+'.png'), kornia.tensor_to_image(OUT_STAK[i].cpu().byte())) #Creare cartelle 

def compute(CT, PET, name, curr_res, t_id, patient_id, filename,volume):
    for _ in range(1):
        final_img=[]
        times=[]
        t = 0.0
        it_time = 0.0
        hist_dim = 256
        dim = 512
        global ref_vals
        ref_vals = torch.ones(dim*dim*volume, dtype=torch.int, device=device)
        global move_data
        move_data = no_transfer if device=='cpu' else to_cuda
        refs = []
        flts = []
        couples = 0
        for c,ij in enumerate(zip(CT, PET)):
            i = ij[0]
            j = ij[1]  
            ref = pydicom.dcmread(i)
            Ref_img = torch.tensor(ref.pixel_array.astype(np.int16), dtype=torch.int16, device=device)
            Ref_img[Ref_img==-2000]=1
            
            flt = pydicom.dcmread(j)
            Flt_img = torch.tensor(flt.pixel_array.astype(np.int16), dtype=torch.int16, device=device)

            Ref_img = (Ref_img - Ref_img.min())/(Ref_img.max() - Ref_img.min())*255
            Ref_uint8 = Ref_img.round().type(torch.uint8)
                    
            Flt_img = (Flt_img - Flt_img.min())/(Flt_img.max() - Flt_img.min())*255
            Flt_uint8 = Flt_img.round().type(torch.uint8)
        
            refs.append(Ref_uint8)
            flts.append(Flt_uint8)
            couples = couples + 1
            if couples >= volume:
                break
            
        refs3D = torch.cat(refs)
        flts3D = torch.cat(flts)
        refs3D = torch.reshape(refs3D,(volume,512,512))
        flts3D = torch.reshape(flts3D,(volume,512,512))
        start_time = time.time()
        f_img=(register_images(filename, refs3D, flts3D, volume))
        final_img = f_img.cpu()
        end_time= time.time()
        it_time = (end_time - start_time)
        times.append(it_time)
        t=t+it_time
        df = pd.DataFrame([t, np.mean(times), np.std(times)],columns=['Test'+str(patient_id)])#+str(config)accel_id.get_config())])
        times_df = pd.DataFrame(times,columns=['Test'+str(patient_id)])#+str(config)accel_id.get_config())])
        df_path = os.path.join(curr_res,'Time_powll_%02d.csv' % (t_id))
        times_df_path = os.path.join(curr_res,'Img_powll_%02d.csv' % (t_id))
        df.to_csv(df_path, index=False)
        times_df.to_csv(times_df_path, index=False)
        save_data(final_img,PET,curr_res,volume)

def compute_wrapper(args, num_threads=1):
    config=args.config
    
    for k in range(args.offset, args.patient):
        pool = []
        #curr_prefix = args.prefix+str(k)
        curr_prefix = args.prefix
        curr_ct = os.path.join(curr_prefix,args.ct_path)
        curr_pet = os.path.join(curr_prefix,args.pet_path)
        curr_res = os.path.join("",args.res_path)
        os.makedirs(curr_res,exist_ok=True)
        CT=glob.glob(curr_ct+'/*dcm')
        PET=glob.glob(curr_pet+'/*dcm')
        PET.sort(key=lambda var:[int(y) if y.isdigit() else y for y in re.findall(r'[^0-9]|[0-9]+',var)])
        CT.sort(key=lambda var:[int(y) if y.isdigit() else y for y in re.findall(r'[^0-9]|[0-9]+',var)])
        assert len(CT) == len(PET)
        images_per_thread = len(CT) // num_threads
        # print(images_per_thread)
        for i in range(num_threads):
            start = images_per_thread * i
            end = images_per_thread * (i + 1) if i < num_threads - 1 else len(CT)
            name = "t%02d" % (i)
            set_start_method('spawn')
            pool.append(Process(target=compute, args=(CT[start:end], PET[start:end], name, curr_res, i, k, args.filename,args.volume)))
        for t in pool:
            t.start()
        for t in pool:
            t.join()

hist_dim = 256
dim = 512

def main():

    parser = argparse.ArgumentParser(description='Iron software for IR onto a python env')
    parser.add_argument("-pt", "--patient", nargs='?', help='Number of the patient to analyze', default=1, type=int)
    parser.add_argument("-o", "--offset", nargs='?', help='Starting patient to analyze', default=0, type=int)
    parser.add_argument("-cp", "--ct_path", nargs='?', help='Path of the CT Images', default='./')
    parser.add_argument("-pp", "--pet_path", nargs='?', help='Path of the PET Images', default='./')
    parser.add_argument("-rp", "--res_path", nargs='?', help='Path of the Results', default='./')
    parser.add_argument("-t", "--thread_number", nargs='?', help='Number of // threads', default=1, type=int)
    parser.add_argument("-px", "--prefix", nargs='?', help='prefix Path of patients folder', default='./')
    parser.add_argument("-im", "--image_dimension", nargs='?', help='Target images dimensions', default=512, type=int)
    parser.add_argument("-c", "--config", nargs='?', help='prefix Path of patients folder', default='./')
    parser.add_argument("-dvc", "--device", nargs='?', help='Target device', choices=['cpu', 'cuda:0'], default='cpu')
    parser.add_argument("-vol", "--volume", nargs='?', help='Volume',type = int, default=512)
    parser.add_argument("-f", "--filename", nargs='?', help='Filename', default="test.csv")

    
    args = parser.parse_args()
    num_threads=args.thread_number

    patient_number=args.patient
   
    # print(args.config)
    # print(args)

    global compute_metric
    compute_metric = compute_mi
    global precompute_metric
    precompute_metric = precompute_mutual_information

    global device
    device = args.device

    compute_wrapper(args, num_threads)
        



if __name__== "__main__":
    main()

















