/*
MIT License

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
*/

#pragma once
#include <adf.h>
#include "transform/kernel_transform_BL.h"
#include "transform/kernel_transform_BR.h"
#include "transform/kernel_transform_TL.h"
#include "transform/kernel_transform_TR.h"
#include "coefficients/kernel_compute_Ri.h"
#include "coefficients/kernel_compute_Riinv.h"
#include "coefficients/kernel_compute_Rj.h"
#include "coefficients/kernel_compute_Rjinv.h"

// #include "kernel_transform.h"
#include "kernel_adder.h"
#include "kernel_mac_top.h"
#include "kernel_mac_bottom.h"
#include "kernel_merger_1_i.h"
#include "kernel_merger_1_j.h"
	
#include "kernel_merger_2.h"

#include "coordinates/kernel_coordinates.h"

using namespace adf;

#define ENABLE_GRAPH_TRANSFORMATION true
#define ENABLE_GRAPH_COEEFFICIENTS true
#define ENABLE_GRAPH_INTERPOLATION true
#define ENABLE_DEBUG_PLIO false

#define WINDOW_PARAMS_SIZE 4*sizeof(float)
#define WINDOW_COORDS_SIZE 32*sizeof(float)
#define WINDOW_COEFF_SIZE 32*sizeof(float)
#define WINDOW_INDEX_SIZE  32*sizeof(int32)
#define WINDOW_PIXEL_IN_SIZE  64*sizeof(uint8)
#define WINDOW_PIXEL_OUT_SIZE  32*sizeof(uint8)
#define WINDOW_PIXEL_FLOAT_SIZE 32*sizeof(float)
#define WINDOW_COEFFIN_SIZE 4*sizeof(float)
#define WINDOW_MERGER_SIZE 64*sizeof(float)

#define COMMON_RUNTIME_RATIO 0.9

class my_graph: public graph
{

private:
#if ENABLE_GRAPH_COEEFFICIENTS
    kernel k_coordinates_co;
    kernel k_coeff_Ri;
    kernel k_coeff_Ri_inv;
    kernel k_coeff_Rj;
    kernel k_coeff_Rj_inv;
    kernel k_merger_1_i;
    kernel k_merger_1_j;
    kernel k_merger_2;
#endif

#if ENABLE_GRAPH_TRANSFORMATION
    kernel k_transform_TL;
    kernel k_coordinates_tr;
    kernel k_transform_TR;
    kernel k_transform_BL;
    kernel k_transform_BR;
#endif
    
#if ENABLE_GRAPH_INTERPOLATION
    kernel k_mac_top[INT_PE];
    kernel k_mac_bottom[INT_PE];
    kernel k_adder[INT_PE];
#endif

public:
#if ENABLE_GRAPH_COEEFFICIENTS || ENABLE_GRAPH_TRANSFORMATION
    input_plio params;
#endif

#if ENABLE_GRAPH_COEEFFICIENTS
    output_plio R_out;
#endif
    
#if ENABLE_GRAPH_TRANSFORMATION
    output_plio TL;
    output_plio TR;
    output_plio BL;
    output_plio BR;
#endif

#if ENABLE_GRAPH_INTERPOLATION
// input_plio R_in;
    input_plio p_ab[INT_PE];
    input_plio p_cd[INT_PE];
    output_plio result[INT_PE];
    
#if ENABLE_DEBUG_PLIO    
#if ENABLE_GRAPH_COEEFFICIENTS
    output_plio out_coords_co_cc;
    output_plio out_coords_co_rr;
    output_plio Ri;
    output_plio Ri_inv;
    output_plio Rj;
    output_plio Rj_inv;
    output_plio R_i_iinv;
    output_plio R_j_jinv;
#endif

#if ENABLE_GRAPH_TRANSFORMATION
    output_plio out_coords_tr_cc;
    output_plio out_coords_tr_rr;
#endif
    
#if ENABLE_GRAPH_INTERPOLATION
    output_plio p_ab_out;
    output_plio p_cd_out;
#endif
#endif
#endif

    my_graph()
	{
        //
        // --- KERNEL INSTANTIATION ---
        //
#if ENABLE_GRAPH_COEEFFICIENTS
        // k_coordinates
        k_coordinates_co = kernel::create(coordinates);  
        // k_coeff
        k_coeff_Ri     = kernel::create(compute_Ri);
        k_coeff_Ri_inv = kernel::create(compute_Riinv);
        k_coeff_Rj     = kernel::create(compute_Rj);
        k_coeff_Rj_inv = kernel::create(compute_Rjinv);
        // k_merger
        k_merger_1_i = kernel::create(merger_1_i);
        k_merger_1_j = kernel::create(merger_1_j);
        k_merger_2 = kernel::create(merger_2);
#endif

#if ENABLE_GRAPH_TRANSFORMATION
        // k_coordinates 
        k_coordinates_tr = kernel::create(coordinates);
        // k_transform
        k_transform_TL = kernel::create(transform_TL);
        k_transform_TR = kernel::create(transform_TR);
        k_transform_BL = kernel::create(transform_BL);
        k_transform_BR = kernel::create(transform_BR);
#endif

        // mac
#if ENABLE_GRAPH_INTERPOLATION
        for (int i = 0; i < INT_PE; i++) {
            k_mac_top[i] = kernel::create(mac_top);
            k_mac_bottom[i] = kernel::create(mac_bottom);
            k_adder[i] = kernel::create(adder);
        }
#endif


        //
        // --- PLIO INSTANTIATION ---
        //

#if ENABLE_GRAPH_COEEFFICIENTS || ENABLE_GRAPH_TRANSFORMATION
        params = input_plio::create("params", plio_32_bits, "data/params_in.txt");
#endif

#if ENABLE_GRAPH_COEEFFICIENTS
        //R_out = output_plio::create("R_out", plio_128_bits, "data/R_out.txt");
#endif

#if ENABLE_GRAPH_TRANSFORMATION
        TL = output_plio::create("TL", plio_32_bits, "data/TL_out.txt");
        TR = output_plio::create("TR", plio_32_bits, "data/TR_out.txt");
        BL = output_plio::create("BL", plio_32_bits, "data/BL_out.txt");
        BR = output_plio::create("BR", plio_32_bits, "data/BR_out.txt");
#endif

#if ENABLE_GRAPH_INTERPOLATION
        //R_in = input_plio::create("R_in", plio_32_bits, "data/R_in.txt");
        for (int i = 0; i < INT_PE; i++) {
            std::string idx = std::to_string(i+1);
            p_ab[i] = input_plio::create("p_ab_" + idx, plio_128_bits, "data/p_ab_" + idx + ".txt");
            p_cd[i] = input_plio::create("p_cd_" + idx, plio_128_bits, "data/p_cd_" + idx + ".txt");
            result[i] = output_plio::create("result_" + idx, plio_128_bits, "data/result_" + idx + ".txt");
        }
#endif

#if ENABLE_DEBUG_PLIO
#if ENABLE_GRAPH_COEEFFICIENTS
        out_coords_co_cc = output_plio::create("out_coords_co_cc", plio_32_bits, "data/out_coords_co_cc.txt");
        out_coords_co_rr = output_plio::create("out_coords_co_rr", plio_32_bits, "data/out_coords_co_rr.txt");
        Ri     = output_plio::create("Ri",     plio_32_bits, "data/Ri_out.txt");
        Ri_inv = output_plio::create("Ri_inv", plio_32_bits, "data/Ri_inv_out.txt");
        Rj     = output_plio::create("Rj",     plio_32_bits, "data/Rj_out.txt");
        Rj_inv = output_plio::create("Rj_inv", plio_32_bits, "data/Rj_inv_out.txt");
        R_i_iinv = output_plio::create("R_i_iinv", plio_32_bits, "data/R_i_iinv_out.txt");
        R_j_jinv = output_plio::create("R_j_jinv", plio_32_bits, "data/R_j_jinv_out.txt");
#endif

#if ENABLE_GRAPH_TRANSFORMATION
        out_coords_tr_cc = output_plio::create("out_coords_tr_cc", plio_32_bits, "data/out_coords_tr_cc.txt");
        out_coords_tr_rr = output_plio::create("out_coords_tr_rr", plio_32_bits, "data/out_coords_tr_rr.txt");
#endif
        
#if ENABLE_GRAPH_INTERPOLATION
        p_ab_out = output_plio::create("p_ab_out", plio_32_bits, "data/p_ab_out.txt");
        p_cd_out = output_plio::create("p_cd_out", plio_32_bits, "data/p_cd_out.txt");
#endif
#endif


        //
        // --- CONNECTIONS ---
        //

        // PLIO params --> k_coordinates
#if ENABLE_GRAPH_COEEFFICIENTS
        connect<window<WINDOW_PARAMS_SIZE>>(params.out[0], k_coordinates_co.in[0]);
#endif
#if ENABLE_GRAPH_TRANSFORMATION
        connect<window<WINDOW_PARAMS_SIZE>>(params.out[0], k_coordinates_tr.in[0]);
#endif

#if ENABLE_GRAPH_COEEFFICIENTS
        // k_coordinates --> k_coeff
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_co.out[0]), async(k_coeff_Ri.in[0]));
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_co.out[1]), async(k_coeff_Ri.in[1]));
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_co.out[0]), async(k_coeff_Ri_inv.in[0]));
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_co.out[1]), async(k_coeff_Ri_inv.in[1]));
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_co.out[0]), async(k_coeff_Rj.in[0]));
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_co.out[1]), async(k_coeff_Rj.in[1]));
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_co.out[0]), async(k_coeff_Rj_inv.in[0]));
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_co.out[1]), async(k_coeff_Rj_inv.in[1]));
        // k_coeff -> merger_1
        connect<window<WINDOW_COEFF_SIZE>>(async(k_coeff_Ri.out[0]),     async(k_merger_1_i.in[0]));
        connect<window<WINDOW_COEFF_SIZE>>(async(k_coeff_Ri_inv.out[0]), async(k_merger_1_i.in[1]));
        connect<window<WINDOW_COEFF_SIZE>>(async(k_coeff_Rj.out[0]),     async(k_merger_1_j.in[0]));
        connect<window<WINDOW_COEFF_SIZE>>(async(k_coeff_Rj_inv.out[0]), async(k_merger_1_j.in[1]));
#endif

#if ENABLE_GRAPH_TRANSFORMATION
        // k_coordinates --> k_transform
        connect<window<WINDOW_INDEX_SIZE>>(async(k_coordinates_tr.out[0]), async(k_transform_TL.in[0]));
        connect<window<WINDOW_INDEX_SIZE>>(async(k_coordinates_tr.out[1]), async(k_transform_TL.in[1]));
        connect<window<WINDOW_INDEX_SIZE>>(async(k_coordinates_tr.out[0]), async(k_transform_TR.in[0]));
        connect<window<WINDOW_INDEX_SIZE>>(async(k_coordinates_tr.out[1]), async(k_transform_TR.in[1]));
        connect<window<WINDOW_INDEX_SIZE>>(async(k_coordinates_tr.out[0]), async(k_transform_BL.in[0]));
        connect<window<WINDOW_INDEX_SIZE>>(async(k_coordinates_tr.out[1]), async(k_transform_BL.in[1]));
        connect<window<WINDOW_INDEX_SIZE>>(async(k_coordinates_tr.out[0]), async(k_transform_BR.in[0]));
        connect<window<WINDOW_INDEX_SIZE>>(async(k_coordinates_tr.out[1]), async(k_transform_BR.in[1]));
        // k_transform --> PLIO TX_out
        connect<window<WINDOW_INDEX_SIZE>>(async(k_transform_TL.out[0]), TL.in[0]);
        connect<window<WINDOW_INDEX_SIZE>>(async(k_transform_TR.out[0]), TR.in[0]);
        connect<window<WINDOW_INDEX_SIZE>>(async(k_transform_BL.out[0]), BL.in[0]);
        connect<window<WINDOW_INDEX_SIZE>>(async(k_transform_BR.out[0]), BR.in[0]);
#endif

#if ENABLE_GRAPH_COEEFFICIENTS
        // merger_1 -> merger_2
        connect<window<WINDOW_MERGER_SIZE>>(async(k_merger_1_i.out[0]), async(k_merger_2.in[0]));
        connect<window<WINDOW_MERGER_SIZE>>(async(k_merger_1_j.out[0]), async(k_merger_2.in[1]));
        // k_merger_2 --> PLIO R_out
        //connect<window<WINDOW_MERGER_SIZE>>(async(k_merger_2.out[0]), R_out.in[0]);
#endif

#if ENABLE_GRAPH_INTERPOLATION
        // PLIO p_ab, p_cd -> mac
        for (int i = 0; i < INT_PE; i++) {
            connect<window<WINDOW_MERGER_SIZE>>(async(k_merger_2.out[0]), async(k_mac_top[i].in[1]));
            connect<window<WINDOW_MERGER_SIZE>>(async(k_merger_2.out[0]), async(k_mac_bottom[i].in[1]));
            connect<window<WINDOW_PIXEL_IN_SIZE>>(p_ab[i].out[0], async(k_mac_top[i].in[0]));
            connect<window<WINDOW_PIXEL_IN_SIZE>>(p_cd[i].out[0], async(k_mac_bottom[i].in[0]));
        }
        
        
        // mac -> adder
        for (int i = 0; i < INT_PE; i++) {
            connect<window<WINDOW_PIXEL_FLOAT_SIZE>>(async(k_mac_top[i].out[0]), async(k_adder[i].in[0]));
            connect<window<WINDOW_PIXEL_FLOAT_SIZE>>(async(k_mac_bottom[i].out[0]), async(k_adder[i].in[1]));
        }
        
        // adder --> PLIO result
		for (int i = 0; i < INT_PE; i++) {
            connect<stream>(async(k_adder[i].out[0]), result[i].in[0]);
        }
#endif

#if ENABLE_DEBUG_PLIO
        //
        // --- CONNECTIONS FOR DEBUGGING ---
        //

#if ENABLE_GRAPH_COEEFFICIENTS
        // k_coordinates --> PLIO out_coords
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_co.out[0]), out_coords_co_cc.in[0]);
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_co.out[1]), out_coords_co_rr.in[0]);
        // k_coeff --> PLIO R_out
        connect<window<WINDOW_COEFF_SIZE>>(async(k_coeff_Ri.out[0]),     Ri.in[0]);
        connect<window<WINDOW_COEFF_SIZE>>(async(k_coeff_Ri_inv.out[0]), Ri_inv.in[0]);
        connect<window<WINDOW_COEFF_SIZE>>(async(k_coeff_Rj.out[0]),     Rj.in[0]);
        connect<window<WINDOW_COEFF_SIZE>>(async(k_coeff_Rj_inv.out[0]), Rj_inv.in[0]);
        // k_merger_1 --> PLIO R_i_iinv, R_j_jinv
        connect<stream>(async(k_merger_1_i.out[0]), R_i_iinv.in[0]);
        connect<stream>(async(k_merger_1_j.out[0]), R_j_jinv.in[0]);
#endif

#if ENABLE_GRAPH_TRANSFORMATION
        // k_coordinates --> PLIO out_coords
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_tr.out[0]), out_coords_tr_cc.in[0]);
        connect<window<WINDOW_COORDS_SIZE>>(async(k_coordinates_tr.out[1]), out_coords_tr_rr.in[0]);
#endif

#if ENABLE_GRAPH_INTERPOLATION
        // mac --> PLIO p_ab_out, p_cd_out
        connect<window<WINDOW_PIXEL_FLOAT_SIZE>>(async(k_mac_top.out[0]), p_ab_out.in[0]);
        connect<window<WINDOW_PIXEL_FLOAT_SIZE>>(async(k_mac_bottom.out[0]), p_cd_out.in[0]);
#endif
#endif

#if ENABLE_GRAPH_COEEFFICIENTS
        source(k_coordinates_co)  = "src/coordinates/kernel_coordinates.cpp";
        headers(k_coordinates_co) = {"src/coordinates/kernel_coordinates.h","../common/common.h"};
        source(k_coeff_Ri)  = "src/coefficients/kernel_compute_Ri.cpp";
        headers(k_coeff_Ri) = {"src/coefficients/kernel_compute_Ri.h","../common/common.h"};
        source(k_coeff_Ri_inv)  = "src/coefficients/kernel_compute_Riinv.cpp";
        headers(k_coeff_Ri_inv) = {"src/coefficients/kernel_compute_Riinv.h","../common/common.h"};
        source(k_coeff_Rj)  = "src/coefficients/kernel_compute_Rj.cpp";
        headers(k_coeff_Rj) = {"src/coefficients/kernel_compute_Rj.h","../common/common.h"};
        source(k_coeff_Rj_inv)  = "src/coefficients/kernel_compute_Rjinv.cpp";
        headers(k_coeff_Rj_inv) = {"src/coefficients/kernel_compute_Rjinv.h","../common/common.h"};
        source(k_merger_1_i)  = "src/kernel_merger_1_i.cpp";
        headers(k_merger_1_i) = {"src/kernel_merger_1_i.h","../common/common.h"};
        source(k_merger_1_j)  = "src/kernel_merger_1_j.cpp";
        headers(k_merger_1_j) = {"src/kernel_merger_1_j.h","../common/common.h"};
        source(k_merger_2)  = "src/kernel_merger_2.cpp";
        headers(k_merger_2) = {"src/kernel_merger_2.h","../common/common.h"};

        runtime<ratio>(k_coordinates_co) = COMMON_RUNTIME_RATIO;
        runtime<ratio>(k_coeff_Ri) = COMMON_RUNTIME_RATIO;
        runtime<ratio>(k_coeff_Ri_inv) = COMMON_RUNTIME_RATIO;
        runtime<ratio>(k_coeff_Rj) = COMMON_RUNTIME_RATIO;
        runtime<ratio>(k_coeff_Rj_inv) = COMMON_RUNTIME_RATIO;
        runtime<ratio>(k_merger_1_i) = COMMON_RUNTIME_RATIO;
        runtime<ratio>(k_merger_1_j) = COMMON_RUNTIME_RATIO;
        runtime<ratio>(k_merger_2) = COMMON_RUNTIME_RATIO;
#endif

#if ENABLE_GRAPH_TRANSFORMATION
        source(k_coordinates_tr)  = "src/coordinates/kernel_coordinates.cpp";
        headers(k_coordinates_tr) = {"src/coordinates/kernel_coordinates.h","../common/common.h"};
        source(k_transform_TL)  = "src/transform/kernel_transform_TL.cpp";
        headers(k_transform_TL) = {"src/transform/kernel_transform_TL.h","../common/common.h"};
        source(k_transform_TR)  = "src/transform/kernel_transform_TR.cpp";
        headers(k_transform_TR) = {"src/transform/kernel_transform_TR.h","../common/common.h"};
        source(k_transform_BL)  = "src/transform/kernel_transform_BL.cpp";
        headers(k_transform_BL) = {"src/transform/kernel_transform_BL.h","../common/common.h"};
        source(k_transform_BR)  = "src/transform/kernel_transform_BR.cpp";
        headers(k_transform_BR) = {"src/transform/kernel_transform_BR.h","../common/common.h"};

        runtime<ratio>(k_coordinates_tr) = COMMON_RUNTIME_RATIO;
        runtime<ratio>(k_transform_TL) = COMMON_RUNTIME_RATIO;
        runtime<ratio>(k_transform_TR) = COMMON_RUNTIME_RATIO;
        runtime<ratio>(k_transform_BL) = COMMON_RUNTIME_RATIO;
        runtime<ratio>(k_transform_BR) = COMMON_RUNTIME_RATIO;
#endif
        
#if ENABLE_GRAPH_INTERPOLATION
        for (int i = 0; i < INT_PE; i++) {
            source(k_mac_top[i])  = "src/kernel_mac_top.cpp";
            headers(k_mac_top[i]) = {"src/kernel_mac_top.h","../common/common.h"};
            source(k_mac_bottom[i])  = "src/kernel_mac_bottom.cpp";
            headers(k_mac_bottom[i]) = {"src/kernel_mac_bottom.h","../common/common.h"};
            source(k_adder[i])  = "src/kernel_adder.cpp";
            headers(k_adder[i]) = {"src/kernel_adder.h","../common/common.h"};

            runtime<ratio>(k_mac_top[i]) = COMMON_RUNTIME_RATIO;
            runtime<ratio>(k_mac_bottom[i]) = COMMON_RUNTIME_RATIO;
            runtime<ratio>(k_adder[i]) = COMMON_RUNTIME_RATIO;
        }

#endif
    }

};
