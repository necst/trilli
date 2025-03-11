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

#include "common.h"

#include "kernel_coordinates.h"

#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"

using namespace std;

alignas(32) const float init_col_const[32] = INIT_COLS;
constexpr float HALF_DIMENSION = DIMENSION / 2.f;

constexpr int DIMENSION_DIV_32 = DIMENSION / 32;

void coordinates(input_window_float* restrict params_in, output_window_float* restrict coords_out_cc, output_window_float* restrict coords_out_rr) {
    // configuration of the volume transform
    aie::vector<float, 4> tmp = window_readincr_v4(params_in);
    const float tx = tmp.get(0); // window_readincr(params_in);
    const float ty = tmp.get(1); //window_readincr(params_in);
    const float ang = tmp.get(2); //window_readincr(params_in);
    const float n_couples_float = tmp.get(3); //window_readincr(params_in);
    const int n_couples = aie::to_fixed(n_couples_float, 0);

    // pass configuration to the next kernels
    // aie::vector<float, 32> coords_out_cc_v = aie::broadcast<float, 32>(0.f);
    // coords_out_cc_v[0] = tx;
    // coords_out_cc_v[1] = ty;
    // coords_out_cc_v[2] = ang;
    // coords_out_cc_v[3] = n_couples;

    window_acquire(coords_out_cc);
    window_writeincr(coords_out_cc, tmp);
    window_release(coords_out_cc);
    chess_separator_scheduler();

    // constant values used during transformation phase
    // const aie::vector<float, 32> tx_v = aie::broadcast<float, 32>(tx);
    // const aie::vector<float, 32> ty_v = aie::broadcast<float, 32>(ty);
    // const aie::vector<float, 32> to_center = aie::broadcast<float, 32>(HALF_DIMENSION);
    // const aie::vector<float, 32> from_center_tx = aie::sub(to_center, tx_v);
    // const aie::vector<float, 32> from_center_ty = aie::sub(to_center, ty_v);
    const float p_cos = cos(ang);
    const float p_sin = sin(ang);
    const float n_sin = -p_sin;
    chess_separator_scheduler();

    // constant values used to compute buffer offset from the (c,r) coordinates
    const aie::vector<int32, 32> n_couples_v = aie::broadcast<int32, 32>(n_couples);
    const aie::vector<int32, 32> size_n_couples_v = aie::broadcast<int32, 32>(DIMENSION * n_couples);

    // counters for the columns
    aie::vector<float, 32> init_cols = aie::load_v<32>(init_col_const);
    aie::vector<float, 32> cc_interval = aie::broadcast<float, 32>(32.f);

    // float delta_row_fixed_row = p_sin;
    // float delta_col_fixed_row = p_cos ;
    const float p_cos_32 = p_cos * 32;
    const float p_sin_32 = p_sin * 32;

    // float delta_row_fixed_col = p_cos;
    // float delta_col_fixed_col = n_sin;

    

    aie::vector<float, 32> init_cols_times_p_cos = aie::mul(aie::sub(init_cols, tx), p_cos);
    aie::vector<float, 32> cc_first_tra = aie::add(
            (HALF_DIMENSION + ty) * p_sin + HALF_DIMENSION - p_cos_32, 
            init_cols_times_p_cos
        );

    // rr_first_tra = (0 - SIZE/2.f) * p_sin + (0 - SIZE/2.f) * p_cos + (SIZE/2.f) - TY - delta_row_fixed_row

    // rr_first_tra = (INIT_COLS - SIZE/2.f - TX) * p_sin + (0 - SIZE/2.f - TY) * p_cos + (SIZE/2.f) - delta_row_fixed_row

    //aie::vector<float, 32> rr_first_tra = aie::broadcast<float, 32>((-DIMENSION / 2.f) * p_sin + (-DIMENSION / 2.f) * p_cos + (DIMENSION / 2.f) - ty - delta_row_fixed_row);
    aie::vector<float, 32> init_rows_times_p_sin = aie::mul(aie::sub(init_cols, tx), p_sin);
    aie::vector<float, 32> rr_first_tra = aie::add(
            (-HALF_DIMENSION - ty) * p_cos + HALF_DIMENSION - p_sin_32, 
            init_rows_times_p_sin
        );

    aie::vector<float, 32> cc_tra = aie::broadcast<float, 32>(0.f);
    aie::vector<float, 32> rr_tra = aie::broadcast<float, 32>(0.f);

    // int k_index = 0;

    // loop_rows: for (int r = -HALF_DIMENSION+k_index*SIZE_ROWS; r < -HALF_DIMENSION+(k_index+1)*SIZE_ROWS; r++)
    loop_rows: for (int r = -HALF_DIMENSION; r < -HALF_DIMENSION+SIZE_ROWS; r++) 

    chess_loop_range(32,)
    chess_prepare_for_pipelining
    {
        cc_tra = cc_first_tra;
        cc_first_tra = aie::add(cc_first_tra, n_sin);

        rr_tra = rr_first_tra;
        rr_first_tra = aie::add(rr_first_tra, p_cos);
        chess_separator_scheduler();

        loop_columns: for (int c = 0; c < DIMENSION_DIV_32; c++)
        chess_loop_range(32,)
        chess_prepare_for_pipelining
        {
            cc_tra = aie::add(cc_tra, p_cos_32);
            rr_tra = aie::add(rr_tra, p_sin_32);
            chess_separator_scheduler();

            // 4) write output
            window_acquire(coords_out_cc);
            window_writeincr(coords_out_cc, cc_tra);
            window_release(coords_out_cc);

            window_acquire(coords_out_rr);
            window_writeincr(coords_out_rr, rr_tra);
            window_release(coords_out_rr);
            
            
            // aie::store_unaligned_v((float*)coords_out_cc->ptr, cc_tra);
            // aie::store_unaligned_v((float*)coords_out_rr->ptr, rr_tra);

            //cc = aie::add(cc, cc_interval);
        }

    }
}