""""
"MIT License

Copyright (c) 2025 Giuseppe Sorrentino, Paolo Salvatore Galfano, Davide Conficconi, Eleonora D'Arnese

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

from pickle import TRUE
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import warnings
import argparse

warnings.simplefilter(action='ignore', category=FutureWarning)
plt.rcdefaults()

plt.rcParams.update({
    "text.usetex": True,
    "font.family": "serif",
    "font.serif": ["Palatino"],
})
plt.rcParams["font.size"] = 12
plt.rcParams["xtick.labelsize"]= 12    # major tick size in points
plt.rcParams["ytick.labelsize"]= 12    # major tick size in points
plt.rcParams["legend.fontsize"]= 12   # major tick size in points
plt.rcParams["legend.handletextpad"]=0.01    # major tick size in points

plt.rcParams['hatch.linewidth'] = 0.6
plt.rcParams['axes.labelpad'] = 0
plt.rcParams['pdf.fonttype'] = 42
plt.rcParams['ps.fonttype'] = 42
scale_points=1.75

# Set up argument parsing
parser = argparse.ArgumentParser(description="Process TRILLI file paths.")

parser.add_argument("--trilli_tx", type=str, default="./csv/TRILLI_32_IPE.csv",
                    help="Path to TRILLI_TX file (default: ./csv/TRILLI_32_IPE.csv)")

parser.add_argument("--trilli_3dir", type=str, default="./csv/TRILLi_pow.csv",
                    help="Path to TRILLI_POW file (default: ./csv/TRILLi_pow.csv)")

# Parse the arguments
args = parser.parse_args()

# Assign variables
TRILLI_TX = args.trilli_tx
TRILLI_POW = args.trilli_3dir



data_vitisLib = pd.read_csv("./csv/VitisLibrary_246_ExecTime.csv")
df_vitisLib = pd.DataFrame(data_vitisLib, columns=["Config", "Time"])


data_itk = pd.read_csv("./csv/itk_u7_o3.csv")
data_itk = data_itk.rename(columns={"TxTime": "Time"})
df_itk = pd.DataFrame(data_itk, columns=["Index","Time", "Config", "MITime", "TxTime"])

data_matlab = pd.read_csv("./csv/matlab_i7_11.csv")
df_matlab = pd.DataFrame(data_matlab, columns=["tx"])
df_matlab = df_matlab.rename(columns={"tx": "Time"})

data_hephaestus = pd.read_csv("./csv/hephaestus.csv")
data_hephaestus = data_hephaestus.rename(columns={"TxTime": "Time"})
df = pd.DataFrame(data_hephaestus, columns=["Index","Time", "Config", "MITime", "TxTime"])

data_A5000 = pd.read_csv("./csv/A5000_output_bilinear_512.csv")
df_A5000 = pd.DataFrame(data_A5000, columns=["tot", "tx", "mi"])
df_A5000 = df_A5000.rename(columns={"tx": "Time"})

data_A100 = pd.read_csv("./csv/A100_output_bilinear_512.csv")
df_A100 = pd.DataFrame(data_A100, columns=["tot", "tx", "mi"])
df_A100 = df_A100.rename(columns={"tx": "Time"})

data_RTX = pd.read_csv("./csv/RTX4050_output_bilinear_512.csv")
df_RTX = pd.DataFrame(data_RTX, columns=["tot", "tx", "mi"])
df_RTX = df_RTX.rename(columns={"tx": "Time"})

data_VCK= pd.read_csv(TRILLI_TX)
df_VCK = pd.DataFrame(data_VCK, columns=["exec_time", "write_time", "read_time"])
df_VCK = df_VCK.rename(columns={"exec_time": "Time"})

df_A100["Config"] = "Kornia\_A100"
df_A5000["Config"] = "Kornia\_A5000"
df_RTX["Config"] = "Kornia\_RTX4050"
df_VCK["Config"] = "TRILLI\_VCK5000"
df_matlab["Config"] = "Matlab\_i7"
df_itk["Config"] = "ITK\_u7\_o3"
data_hephaestus["Config"] = "Hephaestus\n(Kornia\_CPU)"
df_vitisLib["Config"] = "VitisLib\_vck5000"

df_tot = pd.concat([df_itk[["Config", "Time"]] , df_matlab[["Config", "Time"]], data_hephaestus[["Config", "Time"]], df_vitisLib[["Config","Time"]], df_A5000[["Config", "Time"]], df_A100[["Config", "Time"]], df_RTX[["Config", "Time"]], df_VCK[["Config", "Time"]]])
                # itk ,     matlab,  hephaestus, VitisLib,  a5000,    a100,    rtx4050,   TRILLi
custom_palette_1 = ["#b35806","#e08214","#8073ac","#b2abd2","#b30000","#ef6548","#fc8d59","#1a9850"]
custom_pattern_1 = ["//","//","x","x",".",".",".",""]

ax1 = sns.barplot(x="Config", y="Time", data=df_tot, palette=custom_palette_1,edgecolor="black")
ax1.set_ylabel("Execution Time [ms]")
ax1.set_xlabel("")
ax1.set_yscale("log")
plt.xticks(rotation=35)
ax1.spines['top'].set_visible(False)
ax1.spines['right'].set_visible(False)
for i, bar in enumerate(ax1.patches):
    bar.set_hatch(custom_pattern_1[i])

data_itk = pd.read_csv("./csv/itk_u7_o3.csv")
data_itk = data_itk.rename(columns={"ExeTime": "Time"})
df_itk = pd.DataFrame(data_itk, columns=["Index","Time", "Config", "MITime", "TxTime"])

data_hephaestus = pd.read_csv("./csv/hephaestus.csv")
data_hephaestus["Time"] = data_hephaestus["ExeTime"]
df = pd.DataFrame(data_hephaestus, columns=["Index","Time", "Config", "MITime", "TxTime"])

#aggiungi matlab
data_matlab = pd.read_csv("./csv/matlab_i7_11.csv")
df_matlab = pd.DataFrame(data_matlab, columns=["tot"])
df_matlab = df_matlab.rename(columns={"tot": "Time"})

data_A100 = pd.read_csv("./csv/A100_output_bilinear_512.csv")
df_A100 = pd.DataFrame(data_A100, columns=["tot", "tx", "mi"])
df_A100 = df_A100.rename(columns={"tot": "Time"})

data_A5000 = pd.read_csv("./csv/A5000_output_bilinear_512.csv")
df_A5000 = pd.DataFrame(data_A5000, columns=["tot", "tx", "mi"])
df_A5000 = df_A5000.rename(columns={"tot": "Time"})

data_RTX = pd.read_csv("./csv/RTX4050_output_bilinear_512.csv")
df_RTX = pd.DataFrame(data_RTX, columns=["tot", "tx", "mi"])
df_RTX = df_RTX.rename(columns={"tot": "Time"})

data_VCK= pd.read_csv(TRILLI_TX)
df_VCK = pd.DataFrame(data_VCK, columns=["exec_time", "write_time", "read_time"])
df_VCK = df_VCK.rename(columns={"exec_time": "Time"})

df_A100["Config"] = "Athena\_A100"
df_A5000["Config"] = "Athena\_A5000"
df_RTX["Config"] = "Athena\_RTX4050"
df_VCK["Config"] = "TRILLI\_VCK5000"
data_hephaestus["Config"] = "Hephaestus\nCPU + Alveo U280 "
df_matlab["Config"] = "Matlab\_i7"
df_itk["Config"] = "ITK\_u7\_o3"

                #    itk     matlab,   hephaestus, a5000,    a100,    rtx4050,   TRILLi
custom_palette_2 = ["#b35806","#e08214","#8073ac","#b30000","#ef6548","#fc8d59","#1a9850"]
custom_pattern_2 = ["//","//","x",".",".",".",""]

df_tot_2 = pd.concat([df_itk[["Config", "Time"]] , df_matlab[["Config", "Time"]], data_hephaestus[["Config", "Time"]], df_A5000[["Config", "Time"]], df_A100[["Config", "Time"]], df_RTX[["Config", "Time"]], df_VCK[["Config", "Time"]]])

ax2 = sns.barplot(x="Config", y="Time", data=df_tot_2, palette=custom_palette_2,edgecolor="black")
ax2.set_ylabel("Execution Time [ms]")
ax2.set_xlabel("")
ax2.set_yscale("log")
plt.xticks(rotation=35)

ax2.spines['top'].set_visible(False)
ax2.spines['right'].set_visible(False)

totDataframe = pd.DataFrame()

dataITKpowell = pd.read_csv('./csv/itk_pow_estimate_o3.csv')
dataITKpowell = pd.DataFrame(dataITKpowell,columns = ["Time"])
dataITKpowell["index"] = 1
dataITKpowell["Sw"] = "ITK\_U7155H"
totDataframe = pd.concat([totDataframe, dataITKpowell], ignore_index=True)

dataSitkLince = pd.read_csv('./csv/TimeSitkpowi7.csv')
dataSitkLince = pd.DataFrame(dataSitkLince,columns = ["Time"])
dataSitkLince["index"] = 1
dataSitkLince["Sw"] = "SITK-I7"
varianzaSitkLince =  np.std(dataSitkLince['Time'].to_list())
tempoMedioSitkLince =  np.mean(dataSitkLince['Time'].to_list())
totDataframe = pd.concat([totDataframe, dataSitkLince], ignore_index=True)

dataHephaestus = pd.read_csv('./csv/TimePow_u280_8pe_16pen_2C_cache_noprint.csv')
dataHephaestus = pd.DataFrame(dataHephaestus,columns = ["Time"])
dataHephaestus["index"] = 1
dataHephaestus["Sw"] = "Hephaestus"
varianzaHephaestus =  np.std(dataHephaestus['Time'].to_list())
tempoMedioHephaestus =  np.mean(dataHephaestus['Time'].to_list())
totDataframe = pd.concat([totDataframe, dataHephaestus], ignore_index=True)

dataA5000 = pd.read_csv('./csv/Powell_classicMoments_ampere.csv')
dataA5000 = pd.DataFrame(dataA5000,columns = ["Time"])
dataA5000["index"] = 1
dataA5000["Sw"] = "Athena_A5000"
varianzaA5000 =  np.std(dataA5000['Time'].to_list())
tempoMedioA5000 =  np.mean(dataA5000['Time'].to_list())
totDataframe = pd.concat([totDataframe, dataA5000], ignore_index=True)

dataPowV100 = pd.read_csv('./csv/Powell_classicMoments_mem_constr_oci.csv')
dataPowV100 = pd.DataFrame(dataPowV100,columns = ["Time"])
dataPowV100["index"] = 1
dataPowV100["Sw"] = "Athena_V100"
varianzaV100 =  np.std(dataPowV100['Time'].to_list())
tempoMedioV100 =  np.mean(dataPowV100['Time'].to_list())

totDataframe = pd.concat([totDataframe, dataPowV100], ignore_index=True)

dataPowA100 = pd.read_csv('./csv/TimePowell_A100.csv')
dataPowA100 = pd.DataFrame(dataPowA100,columns = ["Time"])
dataPowA100["index"] = 1
dataPowA100["Sw"] = "Athena_A100"
varianzaA100 =  np.std(dataPowA100['Time'].to_list())
tempoMedioA100 =  np.mean(dataPowA100['Time'].to_list())

totDataframe = pd.concat([totDataframe, dataPowA100], ignore_index=True)


dataPowRTX4050 = pd.read_csv('./csv/TimePowell_RTX4050.csv')
dataPowRTX4050 = pd.DataFrame(dataPowRTX4050,columns = ["Time"])
dataPowRTX4050["index"] = 1
dataPowRTX4050["Sw"] = "Athena_RTX4050"
varianzaRTX4050 =  np.std(dataPowRTX4050['Time'].to_list())
tempoMedioRTX4050 =  np.mean(dataPowRTX4050['Time'].to_list())

totDataframe = pd.concat([totDataframe, dataPowRTX4050], ignore_index=True)

dataTRILLi = pd.read_csv(TRILLI_POW)
dataTRILLi = pd.DataFrame(dataTRILLi,columns = ["withPCIE_time"])
dataTRILLi["Time"] = dataTRILLi["withPCIE_time"]
dataTRILLi["index"] = 1
dataTRILLi["Sw"] = "TRILLI"
varianzaTRILLi =  np.std(dataTRILLi['Time'].to_list())
tempoMedioTRILLi =  np.mean(dataTRILLi['Time'].to_list())
totDataframe = pd.concat([totDataframe, dataTRILLi], ignore_index=True)

totDataframe = totDataframe[totDataframe['Sw'] != 'Sw_Ryzen7']
plt.figure(figsize=(20, 5))

                #   itk03,  sitk,    hephaestus, a5000,    v100,    a100,    rtx4050,   TRILLi
custom_palette_3 = ["#b35806","#fdb863","#8073ac","#b30000","#d7301f","#ef6548","#fc8d59","#1a9850"]
custom_pattern_3 = ["//","//","x",".",".",".",".",""]
ax3 = sns.catplot(data=totDataframe, kind="bar",x="Sw", y="Time",
    legend = False,edgecolor = 'black', aspect=1.7, height=4, palette = custom_palette_3)

ax3.set_axis_labels("", "Time [s]","", fontsize = 18)
ax3.set_xticklabels(rotation=30, fontsize = 15)
ax3.set_yticklabels(fontsize = 18)
ax3.set(yscale="log")

for i, bar in enumerate(ax3.ax.patches):
    bar.set_hatch(custom_pattern_3[i])

fig, axs = plt.subplots(1, 3, figsize=(24, 4))

custom_palette_legends = ["#b35806", "#e08214", "#fdb863", "#b2abd2", "#8073ac", 
                          "#b30000", "#d7301f", "#ef6548", "#fc8d59", "#1a9850"]
config_names_legends = [
    r"\parbox[c]{4cm}{\centering ITK\_O3\\Intel U755H}",
    r"\parbox[c]{4cm}{\centering \textsc{matlab}\\Intel U755H}",
    r"\parbox[c]{4cm}{\centering SimpleITK\\Intel i7-4770}",
    r"\parbox[c]{4cm}{\centering VitisLib\\Versal VCK5000}",
    r"\parbox[c]{4cm}{\centering HEPHAESTUS\\Alveo U280}",
    r"\parbox[c]{4cm}{\centering Kornia (a) - ATHENA (b-c) NVIDIA A5000}",
    r"\parbox[c]{4cm}{\centering ATHENA\\NVIDIA V100}",
    r"\parbox[c]{4cm}{\centering Kornia (a) - ATHENA (b-c) NVIDIA A100}",
    r"\parbox[c]{4cm}{\centering Kornia (a) - ATHENA (b-c) NVIDIA RTX4050}",
    r"\parbox[c]{4cm}{\centering TRILLi\\Versal VCK5000}"
]
ax1 = sns.barplot(x="Config", y="Time", data=df_tot, palette=custom_palette_1,edgecolor="black", ax=axs[0])

ax2 = sns.barplot(x="Config", y="Time", data=df_tot_2, palette=custom_palette_2,edgecolor="black", ax=axs[1])

ax3 = sns.barplot(x="Sw", y="Time", data=totDataframe, palette=custom_palette_3,edgecolor="black", ax=axs[2])

ax1.set_ylabel("Execution Time [s]", fontsize=18)
ax1.set_xlabel("(a) - Transformation \& Interpolation", fontsize=18, labelpad=10)
ax1.set_yscale("log")
ax1.tick_params(axis='y', labelsize=16)  
ax1.spines['top'].set_visible(False)
ax1.spines['right'].set_visible(False)

for i, bar in enumerate(ax1.patches):
    bar.set_hatch(custom_pattern_1[i])

ax2.set_ylabel("")
ax2.set_xlabel("(b) - Registration Step (TX + MI)", fontsize=18, labelpad=10)
ax2.set_yscale("log")
ax2.tick_params(axis='y', labelsize=16)  
ax2.spines['top'].set_visible(False)
ax2.spines['right'].set_visible(False)
for i, bar in enumerate(ax2.patches):
    bar.set_hatch(custom_pattern_2[i])

ax3.set_ylabel("")
ax3.set_xlabel("(c) Complete 3D Image Registration", fontsize=18 , labelpad=10)
ax3.set_yscale("log")
ax3.tick_params(axis='y', labelsize=16) 
ax3.spines['top'].set_visible(False)
ax3.spines['right'].set_visible(False)
for i, bar in enumerate(ax3.patches):
        bar.set_hatch(custom_pattern_3[i])

plt.setp(axs[0].get_xticklabels(), visible=False)
plt.setp(axs[1].get_xticklabels(), visible=False)
plt.setp(axs[2].get_xticklabels(), visible=False)

empty_label = ""
empty_handles = plt.Line2D([0], [0], color='none', label=empty_label)


custom_palette_legends = ["#b35806", "#e08214", "#fdb863", "#b2abd2", "#8073ac", 
                      "#b30000", "#d7301f", "#ef6548", "#fc8d59", "#1a9850"]

color1 = plt.Rectangle((0, 0), 3, 2, fc="#b35806", edgecolor = 'black', hatch='//')
color2 = plt.Rectangle((0, 0), 3, 2, fc="#e08214", edgecolor = 'black', hatch='//')
color3 = plt.Rectangle((0, 0), 3, 2, fc="#fdb863", edgecolor = 'black', hatch='//')
color4 = plt.Rectangle((0, 0), 3, 2, fc="#8073ac", edgecolor = 'black', hatch='x')
color5 = plt.Rectangle((0, 0), 3, 2, fc="#b2abd2", edgecolor = 'black', hatch='x')
color6 = plt.Rectangle((0, 0), 3, 2, fc="#b30000", edgecolor = 'black', hatch='.')
color7 = plt.Rectangle((0, 0), 3, 2, fc="#d7301f", edgecolor = 'black', hatch='.')
color8 = plt.Rectangle((0, 0), 3, 2, fc="#ef6548", edgecolor = 'black', hatch='.')
color9 = plt.Rectangle((0, 0), 3, 2, fc="#fc8d59", edgecolor = 'black', hatch='.')
color10 =plt.Rectangle((0, 0), 3, 2, fc="#1a9850", edgecolor = 'black', hatch='')
label1 = r"\parbox[c]{4cm}{\centering ITK\_O3\\Intel U755H}"
label2 = r"\parbox[c]{4cm}{\centering \textsc{matlab}\\Intel U755H}"
label3 = r"\parbox[c]{4cm}{\centering SimpleITK\\Intel i7-4770}"
label4 = r"\parbox[c]{5cm}{\centering Kornia CPU (a) - \\ HEPHAESTUS (b-c) Alveo U280}"
label5 = r"\parbox[c]{4cm}{\centering VitisLib\\Versal VCK5000}"
label6 = r"\parbox[c]{4cm}{\centering Kornia (a) - ATHENA (b-c) NVIDIA A5000}"
label7 = r"\parbox[c]{4cm}{\centering ATHENA\\NVIDIA V100}"
label8 = r"\parbox[c]{4cm}{\centering Kornia (a) - ATHENA (b-c) NVIDIA A100}"
label9 = r"\parbox[c]{4cm}{\centering Kornia (a) - ATHENA (b-c) NVIDIA RTX4050}"
label10 = r"\parbox[c]{4cm}{\centering TRILLI\\Versal VCK5000}"
handleL = 4
fig.legend([color1], [empty_label], loc='upper center', bbox_to_anchor=(0.11, 1.17), 
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1,handlelength=handleL, handleheight=1.5)
fig.legend([color2], [empty_label], loc='upper center', bbox_to_anchor=(0.18, 1.17),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1,handlelength=handleL, handleheight=1.5)
fig.legend([color3], [empty_label], loc='upper center', bbox_to_anchor=(0.26, 1.17),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1,handlelength=handleL, handleheight=1.5)
fig.legend([color4], [empty_label], loc='upper center', bbox_to_anchor=(0.34, 1.17),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1,handlelength=handleL, handleheight=1.5)
fig.legend([color5], [empty_label], loc='upper center', bbox_to_anchor=(0.42, 1.17),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1,handlelength=handleL, handleheight=1.5)
fig.legend([color6], [empty_label], loc='upper center', bbox_to_anchor=(0.5, 1.17),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1,handlelength=handleL, handleheight=1.5)
fig.legend([color7], [empty_label], loc='upper center', bbox_to_anchor=(0.58, 1.17),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1,handlelength=handleL, handleheight=1.5)
fig.legend([color8], [empty_label], loc='upper center', bbox_to_anchor=(0.66, 1.17),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1,handlelength=handleL, handleheight=1.5)
fig.legend([color9], [empty_label], loc='upper center', bbox_to_anchor=(0.74, 1.17),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1, handlelength=handleL, handleheight=1.5)
fig.legend([color10], [empty_label], loc='upper center', bbox_to_anchor=(0.82, 1.17),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1,handlelength=handleL, handleheight=1.5)
fig.legend([empty_handles], [label1], loc='upper center', bbox_to_anchor=(0.1, 1.10),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=16, 
        title_fontsize=1)
fig.legend([empty_handles], [label2], loc='upper center', bbox_to_anchor=(0.172, 1.10),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=16, 
        title_fontsize=1)
fig.legend([empty_handles], [label3], loc='upper center', bbox_to_anchor=(0.254, 1.10),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=16, 
        title_fontsize=1)
fig.legend([empty_handles], [label4], loc='upper center', bbox_to_anchor=(0.33, 1.10),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=16, 
        title_fontsize=1)
fig.legend([empty_handles], [label5], loc='upper center', bbox_to_anchor=(0.41, 1.09),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1)
fig.legend([empty_handles], [label6], loc='upper center', bbox_to_anchor=(0.494, 1.10),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1)
fig.legend([empty_handles], [label7], loc='upper center', bbox_to_anchor=(0.571, 1.095),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1)
fig.legend([empty_handles], [label8], loc='upper center', bbox_to_anchor=(0.652, 1.095),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1)
fig.legend([empty_handles], [label9], loc='upper center', bbox_to_anchor=(0.73, 1.095),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1)
fig.legend([empty_handles], [label10], loc='upper center', bbox_to_anchor=(0.81, 1.095),
        ncol=1, title="", fancybox=False, frameon=False, shadow=False, fontsize=14, 
        title_fontsize=1)

plt.savefig("unified.pdf", bbox_inches = 'tight')
