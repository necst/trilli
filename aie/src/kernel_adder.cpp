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

#include "kernel_adder.h"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"
using namespace std;

inline aie::vector<uint8,32> fromFloat32ToUint8(aie::vector<float,32> vec) {
    // initial vector: [float-pixel-value, float-pixel-value, ...]
    
    // 1) convert to fixed point (by rounding), getting: [int32-pixel-value, int32-pixel-value, ...]
    aie::vector<int32, 32> int32x32 = aie::to_fixed(vec, 0);
    // 2) cast to uint8, getting: [uint8-pixel-value, 0, 0, 0, uint8-pixel-value, 0, 0, 0, ...]
    aie::vector<uint8, 128> uint8x128 = int32x32.cast_to<uint8>();
    // 3) select even pixels, getting: [uint8-pixel-value, 0, uint8-pixel-value, 0 ...]
    aie::vector<uint8, 64> uint8x64 = aie::filter_even(uint8x128, 1);
    // 4) select again even pixels, getting: [uint8-pixel-value, uint8-pixel-value, ...]
    aie::vector<uint8, 32> uint8x32 = aie::filter_even(uint8x64, 1);

    return uint8x32;
}

void adder(input_window_float* restrict p_ab, input_window_float* restrict p_cd, output_stream<uint8>* restrict float_interpolated){
    aie::vector<float, 32> pixel_ab;
    aie::vector<float, 32> pixel_cd;
    aie::vector<float, 32> pixel_interpolated_float;

    window_acquire(p_ab);
    aie::vector<float, 32> tmp = window_readincr_v32(p_ab);
    window_release(p_ab);
    aie::vector<int32, 32> tmp2 = aie::to_fixed(tmp, 0);
    const int n_couples = tmp2.get(0);
    const int size = (DIMENSION * DIMENSION * n_couples/INT_PE) >> 5; 

    for (int i = 0; i < size; i++)
    chess_loop_range(DIMENSION*DIMENSION,)
    chess_prepare_for_pipelining
    {
        window_acquire(p_ab);
        pixel_ab = window_readincr_v32(p_ab);
        window_release(p_ab);

        window_acquire(p_cd);
        pixel_cd = window_readincr_v32(p_cd);
        window_release(p_cd);

        pixel_interpolated_float = aie::add(pixel_ab, pixel_cd);

        writeincr(float_interpolated, fromFloat32ToUint8(pixel_interpolated_float));
    }
}