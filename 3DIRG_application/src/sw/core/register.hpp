/******************************************
* MIT License
*
* Copyright (c) 2025 Giuseppe Sorrentino, Paolo Salvatore Galfano, Davide
Conficconi, Eleonora D'Arnese
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.

*MIT License
*
*Copyright (c) [2019] [Davide Conficconi, Eleonora D'Arnese, Marco Domenico
Santambrogio]
*
*Permission is hereby granted, free of charge, to any person obtaining a copy
*of this software and associated documentation files (the "Software"), to deal
*in the Software without restriction, including without limitation the rights
*to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*copies of the Software, and to permit persons to whom the Software is
*furnished to do so, subject to the following conditions:
*
*The above copyright notice and this permission notice shall be included in all
*copies or substantial portions of the Software.
*
*THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*SOFTWARE.
*/
/***************************************************************
 *
 * registration calss of the whole app
 * credits goes also to the author of this repo:
 *https://github.com/mariusherzog/ImageRegistration
 *
 ****************************************************************/
#ifndef REGISTER_HPP
#define REGISTER_HPP

#include <iostream>

// include image_utils dalla cartella include
#include "../include/image_utils/image_utils.hpp"

#ifdef HW_REG
#include "../include/versal_3dir/Versal3DIR.cpp"
#else
#include "../include/software_mi/software_mi.cpp"
#endif

#include "optimize.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

/**
 * @brief The registration interface defines the signatures of a registration
 *        operation of a floating image to a reference image.
 */
class registration {
public:
// virtual cv::Mat register_images(cv::Mat ref, cv::Mat flt) = 0;
#ifndef HW_REG
  virtual void register_images_3d(std::vector<cv::Mat> &ref,
                                  std::vector<cv::Mat> &flt,
                                  std::vector<uint8_t> &ref_vol,
                                  std::vector<uint8_t> &flt_vol, int n_couples,
                                  int padding, int rangeX, int rangeY,
                                  float AngZ, uint8_t *registered_volume) = 0;
#else
  virtual void register_images_3d(std::vector<cv::Mat> &ref,
                                  std::vector<cv::Mat> &flt, Versal3DIR &board,
                                  int rangeX, int rangeY, float rangeAngZ) = 0;
#endif
  virtual ~registration() = 0;
};

registration::~registration() {}

/**
 * @brief The mutual information strategy uses mutual information as a
 *        similarity metric for registration.
 * Optimization is performed by Powell's method which iteratively optimizes a
 * parameter in the parameter space one after another.
 * Image moments are used for an initial estimation.
 */
class mutualinformation : public registration {
public:
#ifndef HW_REG
  void register_images_3d(std::vector<cv::Mat> &ref, std::vector<cv::Mat> &flt,
                          std::vector<uint8_t> &ref_vol,
                          std::vector<uint8_t> &flt_vol, int n_couples,
                          int padding, int rangeX, int rangeY, float AngZ,
                          uint8_t *registered_volume) override {

    double tx, ty, a11, a12, a21, a22;
    double avg_tx = 0.0;
    double avg_ty = 0.0;
    double avg_a11 = 0.0;
    double avg_a12 = 0.0;
    double avg_a21 = 0.0;
    double avg_a22 = 0.0;
    for (int i = 0; i < ref.size(); i++) {
      estimate_initial(ref[i], flt[i], tx, ty, a11, a12, a21, a22);
      avg_tx += tx;
      avg_ty += ty;
      avg_a11 += a11;
      avg_a12 += a12;
      avg_a21 += a21;
      avg_a22 += a22;
    }
    avg_tx /= ref.size();
    avg_ty /= ref.size();
    avg_a11 /= ref.size();
    avg_a12 /= ref.size();
    avg_a21 /= ref.size();
    avg_a22 /= ref.size();
    float ang_rad = atan2(avg_a21, avg_a11);
    uint8_t *buffer_ref =
        new uint8_t[DIMENSION * DIMENSION * (n_couples + padding)];
    uint8_t *buffer_flt =
        new uint8_t[DIMENSION * DIMENSION * (n_couples + padding)];
    // copy the reference and floating images to the buffers
    std::memcpy(buffer_ref, ref_vol.data(), ref_vol.size() * sizeof(uint8_t));
    std::memcpy(buffer_flt, flt_vol.data(), flt_vol.size() * sizeof(uint8_t));

    // cast_mats_to_vector(buffer_ref, ref, DIMENSION, n_couples, 0, padding);
    // cast_mats_to_vector(buffer_flt, flt, DIMENSION, n_couples, 0, padding);
    if (std::isnan(avg_tx)) {
      avg_tx = 0.0;
    }
    if (std::isnan(avg_ty)) {
      avg_ty = 0.0;
    }
    if (std::isnan(ang_rad)) {
      ang_rad = 0.0;
    }
    std::vector<double> init{avg_tx, avg_ty, ang_rad};
    std::cout << "initial params -  tx: " << init[0] << " ty: " << init[1]
              << " ang: " << init[2] << std::endl;
    std::vector<double> rng{(double)rangeX, (double)rangeY, (double)AngZ};
    std::pair<std::vector<double>::iterator, std::vector<double>::iterator> o{
        init.begin(), init.end()};
    optimize_powell(o, {rng.begin(), rng.end()},
                    std::bind(cost_function_3d, buffer_ref, buffer_flt,
                              n_couples, padding, std::placeholders::_1));
    std::cout << "optimized params -  tx: " << init[0] << " ty: " << init[1]
              << " ang: " << init[2] << std::endl;
    tx = init[0];
    ty = init[1];
    ang_rad = init[2];
    double mutual_inf =
        sw_registration_step_3d(buffer_ref, buffer_flt, registered_volume,
                                n_couples, tx, ty, ang_rad, n_couples, padding);
  }
#else

  void register_images_3d(std::vector<cv::Mat> &ref, std::vector<cv::Mat> &flt,
                          Versal3DIR &board, int rangeX, int rangeY,
                          float AngZ) override {

    double tx, ty, a11, a12, a21, a22;
    double avg_tx = 0.0;
    double avg_ty = 0.0;
    double avg_a11 = 0.0;
    double avg_a12 = 0.0;
    double avg_a21 = 0.0;
    double avg_a22 = 0.0;
    // average of 2D estimated params
    for (int i = 0; i < ref.size(); i++) {
      estimate_initial(ref[i], flt[i], tx, ty, a11, a12, a21, a22);
      avg_tx += tx;
      avg_ty += ty;
      avg_a11 += a11;
      avg_a12 += a12;
      avg_a21 += a21;
      avg_a22 += a22;
    }
    avg_tx /= ref.size();
    avg_ty /= ref.size();
    avg_a11 /= ref.size();
    avg_a12 /= ref.size();
    avg_a21 /= ref.size();
    avg_a22 /= ref.size();

    float ang_rad = atan2(avg_a21, avg_a11);
    // if some params are nan put 0
    if (std::isnan(avg_tx)) {
      avg_tx = 0.0;
    }
    if (std::isnan(avg_ty)) {
      avg_ty = 0.0;
    }
    if (std::isnan(ang_rad)) {
      ang_rad = 0.0;
    }
    std::vector<double> init{avg_tx, avg_ty, ang_rad};
    std::vector<double> rng{(double)rangeX, (double)rangeY, (double)AngZ};
    std::pair<std::vector<double>::iterator, std::vector<double>::iterator> o{
        init.begin(), init.end()};
    optimize_powell(
        o, {rng.begin(), rng.end()},
        std::bind(cost_function_3d, std::ref(board), std::placeholders::_1));
    tx = init[0];
    ty = init[1];
    ang_rad = init[2];
    double mutual_inf = board.hw_exec_tx(tx, ty, ang_rad, NULL, true);
  }

#endif

private:
#ifdef HW_REG
  static double cost_function_3d(Versal3DIR &board,
                                 std::vector<double>::iterator affine_params) {
    const double tx = affine_params[0];
    const double ty = affine_params[1];
    const double ang_rad = affine_params[2];
    double val = exp(-board.hw_exec(tx, ty, ang_rad));
    return val;
  }
#else
  static double cost_function_3d(uint8_t *ref, uint8_t *flt, int depth,
                                 int padding,
                                 std::vector<double>::iterator affine_params) {
    const double tx = affine_params[0];
    const double ty = affine_params[1];
    const double ang = affine_params[2];
    double partial_mi = exp(
        -sw_registration_step_3d(ref, flt, 512, tx, ty, ang, depth, padding));
    std::cout << "transform params -  tx: " << tx << " ty: " << ty
              << " ang: " << ang << " partial_mi: " << partial_mi << std::endl;
    return partial_mi;
  }
#endif

  static void estimate_initial(cv::Mat ref, cv::Mat flt, double &tx, double &ty,
                               double &a11, double &a12, double &a21,
                               double &a22) {
    cv::Moments im_mom = moments(ref);
    cv::Moments pt_mom = moments(flt);
    cv::Mat ref_bin = ref.clone();
    cv::Mat flt_bin = flt.clone();
    cv::threshold(ref, ref_bin, 40, 256, 0);
    cv::threshold(flt, flt_bin, 40, 256, 0);
    double pt_avg_10 = pt_mom.m10 / pt_mom.m00;
    double pt_avg_01 = pt_mom.m01 / pt_mom.m00;
    double pt_mu_20 = (pt_mom.m20 / pt_mom.m00 * 1.0) - (pt_avg_10 * pt_avg_10);
    double pt_mu_02 = (pt_mom.m02 / pt_mom.m00 * 1.0) - (pt_avg_01 * pt_avg_01);
    double pt_mu_11 = (pt_mom.m11 / pt_mom.m00 * 1.0) - (pt_avg_01 * pt_avg_10);

    double im_avg_10 = im_mom.m10 / im_mom.m00;
    double im_avg_01 = im_mom.m01 / im_mom.m00;
    double im_mu_20 = (im_mom.m20 / im_mom.m00 * 1.0) - (im_avg_10 * im_avg_10);
    double im_mu_02 = (im_mom.m02 / im_mom.m00 * 1.0) - (im_avg_01 * im_avg_01);
    double im_mu_11 = (im_mom.m11 / im_mom.m00 * 1.0) - (im_avg_01 * im_avg_10);

    tx = im_mom.m10 / im_mom.m00 - pt_mom.m10 / pt_mom.m00;
    ty = im_mom.m01 / im_mom.m00 - pt_mom.m01 / pt_mom.m00;
    double rho = 0.5f * atan((2.0 * pt_mu_11) / (pt_mu_20 - pt_mu_02));
    double rho_im = 0.5f * atan((2.0 * im_mu_11) / (im_mu_20 - im_mu_02));

    const double rho_diff = rho_im - rho;

    const double roundness =
        (pt_mom.m20 / pt_mom.m00) / (pt_mom.m02 / pt_mom.m00);
    if (abs(roundness - 1.0) >= 0.3) {
      a11 = cos(rho_diff);
      a12 = -sin(rho_diff);
      a21 = sin(rho_diff);
      a22 = cos(rho_diff);
    } else {
      a11 = 1.0;
      a12 = 0.0;
      a21 = 0.0;
      a22 = 1.0;
    }
  }
};

#endif // REGISTER_HPP
