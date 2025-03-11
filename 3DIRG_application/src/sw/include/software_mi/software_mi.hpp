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
#include <chrono>
#include "../image_utils/image_utils.hpp"
#include "../../../../../common/constants.h"

static double sw_registration_step_3d(uint8_t* input_ref, uint8_t* input_flt, uint8_t* output_flt, int n_couples, const int TX, const int TY, const float ANG,int depth, int padding);
static double sw_registration_step_3d(uint8_t* input_ref, uint8_t* input_flt,int n_couples, const int TX, const int TY, const float ANG,int depth, int padding);
