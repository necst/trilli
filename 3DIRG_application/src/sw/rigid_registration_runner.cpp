#include "rigid_registration_runner.hpp"

#include <chrono>
#include <cstdlib> // getenv
#include <cstring> // strncpy, strcat
#include <fstream>
#include <iostream>

#ifdef HW_REG
#include "experimental/xrt_kernel.h"
#include "experimental/xrt_uuid.h"
#else
#include "./include/software_mi/software_mi.hpp"
#endif

#include "../../../common/constants.h"
#include "app/imagefusion.hpp"
#include "core/fusion_algorithms.hpp"
#include "core/register_algorithms.hpp"

#define DEVICE_ID 0

using namespace cv;
using namespace std;

namespace {

void save_slices_as_png(const std::vector<cv::Mat> &volume,
                        const std::string &folder, const std::string &prefix) {
  for (size_t i = 0; i < volume.size(); ++i) {
    std::string filename =
        folder + "/" + prefix + "_" + std::to_string(i) + ".png";
    cv::imwrite(filename, volume[i]);
  }
  std::cout << "Saved " << volume.size() << " slices to " << folder
            << std::endl;
}

bool get_xclbin_path(std::string &xclbin_file) {
  char *env_emu = getenv("XCL_EMULATION_MODE");
  if (env_emu) {
    std::string mode(env_emu);
    if (mode == "hw_emu") {
      std::cout << "Program running in hardware emulation mode" << std::endl;
      xclbin_file = "overlay_hw_emu.xclbin";
    } else {
      std::cout << "[ERROR] Unsupported Emulation Mode: " << mode << std::endl;
      return false;
    }
  } else {
    std::cout << "\e[1mProgram running in hardware mode\e[0m" << std::endl;
    xclbin_file = "overlay_hw.xclbin";
  }
  std::cout << std::endl << std::endl;
  return true;
}

} // anonymous namespace

void run_rigid_registration_trilli(const std::string &ref_img_path,
                                   const std::string &float_img_path,
                                   const std::string &output_folder,
                                   int n_couples, int rangeX, int rangeY,
                                   float rangeAngZ) {

  std::cout << "Starting Trilli" << std::endl;

  std::cout << "Number of couples: " << n_couples << std::endl;
  std::cout << "RangeX: " << rangeX << std::endl;
  std::cout << "RangeY: " << rangeY << std::endl;
  std::cout << "RangeAngZ: " << rangeAngZ << std::endl;

  int padding = (NUM_PIXELS_PER_READ - (n_couples % NUM_PIXELS_PER_READ)) %
                NUM_PIXELS_PER_READ;
  std::cout << "Padding: " << padding << std::endl;

  file_repository files(ref_img_path, float_img_path);
  std::vector<cv::Mat> reference_image = files.reference_image_3d(n_couples);
  std::vector<cv::Mat> floating_image = files.floating_image_3d(n_couples);
  uint8_t *buffer_ref =
      new uint8_t[DIMENSION * DIMENSION * (n_couples + padding)];
  uint8_t *buffer_flt =
      new uint8_t[DIMENSION * DIMENSION * (n_couples + padding)];
  cast_mats_to_vector(buffer_ref, reference_image, DIMENSION, n_couples, 0,
                      padding);
  cast_mats_to_vector(buffer_flt, floating_image, DIMENSION, n_couples, 0,
                      padding);
  std::vector<uint8_t> ref_vol = std::vector<uint8_t>(
      buffer_ref, buffer_ref + DIMENSION * DIMENSION * (n_couples + padding));
  std::vector<uint8_t> flt_vol = std::vector<uint8_t>(
      buffer_flt, buffer_flt + DIMENSION * DIMENSION * (n_couples + padding));
#ifdef HW_REG
  std::string xclbin_file;
  if (!get_xclbin_path(xclbin_file))
    throw std::runtime_error("Failed to get xclbin path");

  std::cout << "Loading bitstream (" << xclbin_file << ")... ";
  xrt::device device = xrt::device(DEVICE_ID);
  xrt::uuid xclbin_uuid = device.load_xclbin(xclbin_file);
  std::cout << "Done" << std::endl;

  Versal3DIR board(device, xclbin_uuid, n_couples);

  std::ofstream out_file("TRILLI_pow.csv", std::ios::out);
  out_file << "execTime,withPCIE_time" << std::endl;

  if (board.read_volumes_from_file(ref_img_path, float_img_path) == -1)
    throw std::runtime_error("Error reading volumes from file");

  auto start_pcie = std::chrono::high_resolution_clock::now();
  double exec_time = imagefusion::perform_fusion_from_files_3d(
      reference_image, floating_image, "mutualinformation", "alphablend", board,
      rangeX, rangeY, rangeAngZ);
  auto end_pcie = std::chrono::high_resolution_clock::now();

  write_volume_to_file(board.output_flt, DIMENSION, n_couples, 0, padding,
                       output_folder);

  std::cout << "Saving Volumes" << std::endl;

  out_file << exec_time << ","
           << std::chrono::duration<double>(end_pcie - start_pcie).count()
           << std::endl;
  out_file.close();

#else
  uint8_t *registered_volume =
      new uint8_t[DIMENSION * DIMENSION * (n_couples + padding)];

  imagefusion::perform_fusion_from_files_3d(
      reference_image, floating_image, ref_vol, flt_vol, "mutualinformation",
      "alphablend", n_couples, padding, rangeX, rangeY, rangeAngZ,
      registered_volume);

  std::cout << "Saving Volumes" << std::endl;
  write_volume_to_file(registered_volume, DIMENSION, n_couples, 0, padding,
                       output_folder);

  delete[] registered_volume;
#endif
}

std::vector<uint8_t> run_rigid_registration_trilli_from_data(
    std::vector<uint8_t> &ref_volume, std::vector<uint8_t> &float_volume,
    const std::string &output_folder, int n_couples, int rangeX, int rangeY,
    float rangeAngZ) {
  std::cout << "Starting Trilli" << std::endl;

  std::cout << "Number of couples: " << n_couples << std::endl;
  std::cout << "RangeX: " << rangeX << std::endl;
  std::cout << "RangeY: " << rangeY << std::endl;
  std::cout << "RangeAngZ: " << rangeAngZ << std::endl;

  int padding = (NUM_PIXELS_PER_READ - (n_couples % NUM_PIXELS_PER_READ)) %
                NUM_PIXELS_PER_READ;
  std::cout << "Padding: " << padding << std::endl;
  file_repository files(ref_volume, float_volume);
  std::vector<cv::Mat> reference_image =
      files.load_volume_from_raw(ref_volume, n_couples);
  std::vector<cv::Mat> floating_image =
      files.load_volume_from_raw(float_volume, n_couples);
  //
  // save_slices_as_png(reference_image, output_folder + "debug/reference",
  // "ref");
  // save_slices_as_png(floating_image, output_folder + "debug/float",
  // "ref");

#ifdef HW_REG
  std::string xclbin_file;
  if (!get_xclbin_path(xclbin_file))
    throw std::runtime_error("Failed to get xclbin path");
  std::cout << "Loading bitstream (" << xclbin_file << ")... ";
  xrt::device device = xrt::device(DEVICE_ID);
  xrt::uuid xclbin_uuid = device.load_xclbin(xclbin_file);
  std::cout << "Done" << std::endl;
  Versal3DIR board(device, xclbin_uuid, n_couples);
  if (board.load_volumes_from_data(ref_volume, float_volume) == -1)
    throw std::runtime_error("Error reading volumes from data");
  double exec_time = imagefusion::perform_fusion_from_files_3d(
      reference_image, floating_image, "mutualinformation", "alphablend", board,
      rangeX, rangeY, rangeAngZ);
  std::cout << "Returning Volumes - Execution time: " << exec_time << " seconds"
            << std::endl;
  std::vector<uint8_t> output_volume(
      board.output_flt,
      board.output_flt + DIMENSION * DIMENSION * (n_couples + padding));
#else
  uint8_t *registered_volume =
      new uint8_t[DIMENSION * DIMENSION * (n_couples + padding)];
  imagefusion::perform_fusion_from_files_3d(
      reference_image, floating_image, ref_volume, float_volume,
      "mutualinformation", "alphablend", n_couples, padding, rangeX, rangeY,
      rangeAngZ, registered_volume);
  std::cout << "Saving Volumes" << std::endl;
  std::vector<uint8_t> output_volume(
      registered_volume,
      registered_volume + DIMENSION * DIMENSION * (n_couples + padding));
  delete[] registered_volume;
#endif
  return output_volume; // Placeholder return value
}
