/*
MIT License

Copyright (c) 2025 Giuseppe Sorrentino, Paolo Salvatore Galfano, Davide
Conficconi, Eleonora D'Arnese

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

#include "../../../common/common.h"
#include "../image_utils/image_utils.hpp"
#include "experimental/xrt_kernel.h"
#include "experimental/xrt_uuid.h"

// args indexes for setup_aie kernel
#define arg_setup_aie_in_tx 0
#define arg_setup_aie_in_ty 1
#define arg_setup_aie_in_ang 2
#define arg_setup_aie_in_n_couples 3
#define arg_setup_aie_in_n_row 4
#define arg_setup_aie_in_n_col 5

// args indexes for fetcher kernel
#define arg_fetcher_in_flt_original_ptr 2
#define arg_fetcher_in_n_couples 3
#define arg_fetcher_in_n_row 4
#define arg_fetcher_in_n_col 5

// args indexes for setup_interpolation kernel
#define arg_setinterpol_in_n_couples 3 + INT_PE - 1
#define arg_setinterpol_in_n_row 4 + INT_PE - 1
#define arg_setinterpol_in_n_col 5 + INT_PE - 1

// args indexes for writer kernel
#define arg_writer_out_interpolated_ptr 1 + INT_PE - 1
#define arg_writer_in_n_couples 2 + INT_PE - 1
#define arg_writer_in_n_row 3 + INT_PE - 1
#define arg_writer_in_n_col 4 + INT_PE - 1

// args indexes for setup_mi
#define arg_setup_mi_pixel_out INT_PE + 1
#define arg_setup_mi_n_couples INT_PE + 2
#define arg_setup_mi_n_row INT_PE + 3
#define arg_setup_mi_n_col INT_PE + 4

// args mutual info
#define arg_mutual_info_reference 1
#define arg_mutual_info_mi 2
#define arg_mutual_info_input_size 3

class Versal3DIR {
public:
  xrt::device &device;
  xrt::uuid &xclbin_uuid;
  int n_couples;
  int n_row;
  int n_col;
  int row_padding;
  int col_padding;
  int depth_padding;
  size_t buffer_size;

  uint8_t *input_ref = NULL;
  uint8_t *input_flt = NULL;
  uint8_t *output_flt = NULL;

  xrt::kernel krnl_setup_aie;
  xrt::kernel krnl_fetcher_A;
  xrt::kernel krnl_fetcher_B;
  xrt::kernel krnl_fetcher_C;
  xrt::kernel krnl_fetcher_D;
  xrt::kernel krnl_setup_interpol;
  xrt::kernel krnl_setup_interpol_2;
  xrt::kernel krnl_setup_mi;
  xrt::kernel krnl_mutual_info;

  xrtMemoryGroup bank_fetcher_A_flt_in;
  xrtMemoryGroup bank_fetcher_B_flt_in;
  xrtMemoryGroup bank_fetcher_C_flt_in;
  xrtMemoryGroup bank_fetcher_D_flt_in;
  xrtMemoryGroup bank_writer_flt_transformed;
  xrtMemoryGroup bank_setup_mi;
  xrtMemoryGroup bank_mutual_info;
  xrtMemoryGroup bank_mutual_info_output;

  xrt::bo buffer_fetcher_A_flt_in;
  xrt::bo buffer_fetcher_B_flt_in;
  xrt::bo buffer_fetcher_C_flt_in;
  xrt::bo buffer_fetcher_D_flt_in;
  xrt::bo buffer_mutual_info_reference;
  xrt::bo buffer_mutual_info_output;
  xrt::bo buffer_setup_mi_flt_transformed;

  xrt::run run_setup_aie;
  xrt::run run_fetcher_A;
  xrt::run run_fetcher_B;
  xrt::run run_fetcher_C;
  xrt::run run_fetcher_D;
  xrt::run run_setup_interpol;
  xrt::run run_setup_interpol_2;
  xrt::run run_setup_mi;
  xrt::run run_mutual_info;

  //
  // Initialize the board configuring it for a specific volume-depth
  //
  Versal3DIR(xrt::device &device, xrt::uuid &xclbin_uuid, int n_couples,
             int n_row, int n_col)
      : device(device), xclbin_uuid(xclbin_uuid), n_couples(n_couples),
        n_row(n_row), n_col(n_col),
        row_padding((NUM_PIXELS_PER_READ - (n_row % NUM_PIXELS_PER_READ)) %
                    NUM_PIXELS_PER_READ),
        col_padding((NUM_PIXELS_PER_READ - (n_col % NUM_PIXELS_PER_READ)) %
                    NUM_PIXELS_PER_READ),
        depth_padding(
            (NUM_PIXELS_PER_READ - (n_couples % NUM_PIXELS_PER_READ)) %
            NUM_PIXELS_PER_READ),
        buffer_size((n_row + row_padding) * (n_col + col_padding) *
                    (n_couples + depth_padding)) {
    // create kernel objects
    //std::cout << "Creating kernels..." << std::endl;
    krnl_setup_aie = xrt::kernel(device, xclbin_uuid, "setup_aie");
    krnl_fetcher_A = xrt::kernel(device, xclbin_uuid, "fetcher_A");
    krnl_fetcher_B = xrt::kernel(device, xclbin_uuid, "fetcher_B");
    krnl_fetcher_C = xrt::kernel(device, xclbin_uuid, "fetcher_C");
    krnl_fetcher_D = xrt::kernel(device, xclbin_uuid, "fetcher_D");
    krnl_setup_interpol = xrt::kernel(
        device, xclbin_uuid, "setup_interpolator:{setup_interpolator_0}");
    krnl_setup_interpol_2 = xrt::kernel(
        device, xclbin_uuid, "setup_interpolator:{setup_interpolator_1}");
    krnl_setup_mi = xrt::kernel(device, xclbin_uuid, "setup_mi");
    krnl_mutual_info =
        xrt::kernel(device, xclbin_uuid, "mutual_information_master");

    //std::cout << "Kernels created" << std::endl;
    // create memory banks for kernels
    //std::cout << "Creating memory banks..." << std::endl;
    // get memory bank groups for device buffer
    bank_fetcher_A_flt_in =
        krnl_fetcher_A.group_id(arg_fetcher_in_flt_original_ptr);
    bank_fetcher_B_flt_in =
        krnl_fetcher_B.group_id(arg_fetcher_in_flt_original_ptr);
    bank_fetcher_C_flt_in =
        krnl_fetcher_C.group_id(arg_fetcher_in_flt_original_ptr);
    bank_fetcher_D_flt_in =
        krnl_fetcher_D.group_id(arg_fetcher_in_flt_original_ptr);
    bank_setup_mi = krnl_setup_mi.group_id(arg_setup_mi_pixel_out);
    bank_mutual_info = krnl_mutual_info.group_id(arg_mutual_info_reference);
    bank_mutual_info_output = krnl_mutual_info.group_id(arg_mutual_info_mi);
    //std::cout << "Memory banks created" << std::endl;

    // create device buffers
    //std::cout << "Creating device buffers with buffer size: " << buffer_size
    //          << std::endl;
    buffer_fetcher_A_flt_in = xrt::bo(
        device, buffer_size, xrt::bo::flags::normal, bank_fetcher_A_flt_in);
    buffer_fetcher_B_flt_in = xrt::bo(
        device, buffer_size, xrt::bo::flags::normal, bank_fetcher_B_flt_in);
    buffer_fetcher_C_flt_in = xrt::bo(
        device, buffer_size, xrt::bo::flags::normal, bank_fetcher_C_flt_in);
    buffer_fetcher_D_flt_in = xrt::bo(
        device, buffer_size, xrt::bo::flags::normal, bank_fetcher_D_flt_in);
    buffer_setup_mi_flt_transformed =
        xrt::bo(device, buffer_size, xrt::bo::flags::normal, bank_setup_mi);
    buffer_mutual_info_reference =
        xrt::bo(device, buffer_size, xrt::bo::flags::normal, bank_mutual_info);
    buffer_mutual_info_output = xrt::bo(
        device, sizeof(float), xrt::bo::flags::normal, bank_mutual_info_output);
    
    // create kernel runner instances
    run_setup_aie = xrt::run(krnl_setup_aie);
    run_fetcher_A = xrt::run(krnl_fetcher_A);
    run_fetcher_B = xrt::run(krnl_fetcher_B);
    run_fetcher_C = xrt::run(krnl_fetcher_C);
    run_fetcher_D = xrt::run(krnl_fetcher_D);
    run_setup_interpol = xrt::run(krnl_setup_interpol);
    run_setup_interpol_2 = xrt::run(krnl_setup_interpol_2);
    run_setup_mi = xrt::run(krnl_setup_mi);
    run_mutual_info = xrt::run(krnl_mutual_info);

    //std::cout << "Run created" << std::endl;
    // set setup_setminfo kernel arguments
    run_fetcher_A.set_arg(arg_fetcher_in_flt_original_ptr,
                          buffer_fetcher_A_flt_in);
    run_fetcher_A.set_arg(arg_fetcher_in_n_couples, n_couples + depth_padding);
    run_fetcher_A.set_arg(arg_fetcher_in_n_row, n_row + row_padding);
    run_fetcher_A.set_arg(arg_fetcher_in_n_col, n_col + col_padding);
    run_fetcher_B.set_arg(arg_fetcher_in_flt_original_ptr,
                          buffer_fetcher_B_flt_in);
    run_fetcher_B.set_arg(arg_fetcher_in_n_couples, n_couples + depth_padding);
    run_fetcher_B.set_arg(arg_fetcher_in_n_row, n_row + row_padding);
    run_fetcher_B.set_arg(arg_fetcher_in_n_col, n_col + col_padding);
    run_fetcher_C.set_arg(arg_fetcher_in_flt_original_ptr,
                          buffer_fetcher_C_flt_in);
    run_fetcher_C.set_arg(arg_fetcher_in_n_couples, n_couples + depth_padding);
    run_fetcher_C.set_arg(arg_fetcher_in_n_row, n_row + row_padding);
    run_fetcher_C.set_arg(arg_fetcher_in_n_col, n_col + col_padding);
    run_fetcher_D.set_arg(arg_fetcher_in_flt_original_ptr,
                          buffer_fetcher_D_flt_in);
    run_fetcher_D.set_arg(arg_fetcher_in_n_couples, n_couples + depth_padding);
    run_fetcher_D.set_arg(arg_fetcher_in_n_row, n_row + row_padding);
    run_fetcher_D.set_arg(arg_fetcher_in_n_col, n_col + col_padding);

    // set setup_interpol kernel arguments
    run_setup_interpol.set_arg(arg_setinterpol_in_n_couples,
                               n_couples + depth_padding);
    run_setup_interpol.set_arg(arg_setinterpol_in_n_row, n_row + row_padding);
    run_setup_interpol.set_arg(arg_setinterpol_in_n_col, n_col + col_padding);
    run_setup_interpol_2.set_arg(arg_setinterpol_in_n_couples,
                                 n_couples + depth_padding);
    run_setup_interpol_2.set_arg(arg_setinterpol_in_n_row, n_row + row_padding);
    run_setup_interpol_2.set_arg(arg_setinterpol_in_n_col, n_col + col_padding);

    // set setup mi kernel arguments
    run_setup_mi.set_arg(arg_setup_mi_pixel_out,
                         buffer_setup_mi_flt_transformed);
    run_setup_mi.set_arg(arg_setup_mi_n_couples, n_couples + depth_padding);
    run_setup_mi.set_arg(arg_setup_mi_n_row, n_row + row_padding);
    run_setup_mi.set_arg(arg_setup_mi_n_col, n_col + col_padding);

    // set mutual info kernel arguments
    run_mutual_info.set_arg(arg_mutual_info_reference,
                            buffer_mutual_info_reference);
    run_mutual_info.set_arg(arg_mutual_info_mi, buffer_mutual_info_output);
    run_mutual_info.set_arg(arg_mutual_info_input_size,
                            (unsigned int)buffer_size);
  }
  /*
  int load_volumes_from_data(const std::vector<uint8_t> &ref_volume,
                                         const std::vector<uint8_t>
  &float_volume)
  {
      // Calcola la dimensione totale del buffer con padding
      size_t buffer_size =
          static_cast<size_t>(n_row + row_padding) *
          static_cast<size_t>(n_col + col_padding) *
          static_cast<size_t>(n_couples + depth_padding);

      // Alloca i buffer della classe
      input_ref  = new uint8_t[buffer_size];
      input_flt  = new uint8_t[buffer_size];
      output_flt = new uint8_t[buffer_size];

      // Usando :: sto chiamando la funzione globale, che ha lo stesso nome del
  metodo della classe return ::load_volumes_from_data(input_ref, input_flt,
  output_flt, ref_volume, float_volume, n_row, n_col, n_couples, row_padding,
  col_padding, depth_padding);
  }
  */

int load_volumes_from_data(const std::vector<uint8_t> &ref_volume,
                           const std::vector<uint8_t> &float_volume) {
  // Controlla che i vettori abbiano la dimensione giusta
  if (ref_volume.size() != buffer_size ||
      float_volume.size() != buffer_size) {
    std::cerr << "Error: input vectors have wrong size!" << std::endl;
    return -1;
  }

  input_ref = const_cast<uint8_t*>(ref_volume.data());
  input_flt = const_cast<uint8_t*>(float_volume.data());
  //if (!output_flt)
  //  output_flt = new uint8_t[buffer_size];
  //std::memset(output_flt, 0, buffer_size);

  return 0;
}
/*
int load_volumes_from_data(const std::vector<uint8_t> &ref_volume,
                             const std::vector<uint8_t> &float_volume) {

    // Alloca i buffer della classe se non già allocati
    if (!input_ref)
      input_ref = new uint8_t[buffer_size];
    if (!input_flt)
      input_flt = new uint8_t[buffer_size];
    if (!output_flt)
      output_flt = new uint8_t[buffer_size];

    // Controlla che i vettori abbiano la dimensione giusta
    if (ref_volume.size() != buffer_size ||
        float_volume.size() != buffer_size) {
      std::cerr << "Error: input vectors have wrong size!" << std::endl;
      return -1;
    }

    // Copia i dati direttamente
    std::memcpy(input_ref, ref_volume.data(), buffer_size);
    std::memcpy(input_flt, float_volume.data(), buffer_size);

    // Inizializza l'output a zero
    std::memset(output_flt, 0, buffer_size);

    return 0; // Successo
  }
*/  
  //
  // Read volumes from file
  //
  int read_volumes_from_file(const std::string &path_ref,
                             const std::string &path_flt,
                             const ImageFormat imageFormat = ImageFormat::PNG) {
    input_flt = new uint8_t[buffer_size];
    output_flt = new uint8_t[buffer_size];
    input_ref = new uint8_t[buffer_size];
    if (read_volume_from_file(input_flt, n_row, n_col, n_couples, row_padding,
                              col_padding, depth_padding, path_flt,
                              imageFormat) == -1) {
      std::cerr << "Error: Could not open floating volume. Some file in path \""
                << path_flt << "\" might not exist" << std::endl;
      return -1;
    }
    if (read_volume_from_file(input_ref, n_row, n_col, n_couples, row_padding,
                              col_padding, depth_padding, path_ref,
                              imageFormat) == -1) {
      std::cerr
          << "Error: Could not open reference volume. Some file in path \""
          << path_ref << "\" might not exist" << std::endl;
      return -1;
    }
    return 0;
  }

  //
  // Set the transformation parameters
  //
  void set_transform_params(float TX, float TY, float ANG) {
    // set setup_aie kernel arguments
    run_setup_aie.set_arg(arg_setup_aie_in_tx, TX);
    run_setup_aie.set_arg(arg_setup_aie_in_ty, TY);
    run_setup_aie.set_arg(arg_setup_aie_in_ang, ANG);
    run_setup_aie.set_arg(arg_setup_aie_in_n_couples,
                          n_couples + depth_padding);
    run_setup_aie.set_arg(arg_setup_aie_in_n_row, n_row + row_padding);
    run_setup_aie.set_arg(arg_setup_aie_in_n_col, n_col + col_padding);
  }

  //
  // Transform the floating volume to the board
  //
  void write_floating_volume(double *duration = NULL) {
    Timer timer_transfer_flt_write;
    if (duration != NULL)
      timer_transfer_flt_write.start();
    buffer_fetcher_A_flt_in.write(input_flt);
    buffer_fetcher_A_flt_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    buffer_fetcher_B_flt_in.write(input_flt);
    buffer_fetcher_B_flt_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    buffer_fetcher_C_flt_in.write(input_flt);
    buffer_fetcher_C_flt_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    buffer_fetcher_D_flt_in.write(input_flt);
    buffer_fetcher_D_flt_in.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    if (duration != NULL)
      *duration += timer_transfer_flt_write.getElapsedSeconds();
  }

  //
  // Transform the reference volume to the board
  //
  void write_reference_volume(double *duration = NULL) {
    // std::printf("no reference volume needs to be passed to the board\n");
    Timer timer_transfer_ref_write;
    if (duration != NULL)
      timer_transfer_ref_write.start();
    buffer_mutual_info_reference.write(input_ref);
    buffer_mutual_info_reference.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    if (duration != NULL)
      *duration += 0; // timer_transfer_ref_write.getElapsedSeconds();
  }

  //
  // Run the kernels
  //
  void run(double *duration = NULL) {
    // run the pl kernels
    Timer timer_execution;
    if (duration != NULL)
      timer_execution.start();
    run_setup_aie.start();
    run_fetcher_A.start();
    run_fetcher_B.start();
    run_fetcher_C.start();
    run_fetcher_D.start();
    run_setup_interpol.start();
    run_setup_interpol_2.start();
    run_setup_mi.start();
    run_mutual_info.start();

    run_mutual_info.wait();
    run_setup_mi.wait();
    run_setup_interpol.wait();
    run_setup_interpol_2.wait();
    run_fetcher_A.wait();
    run_fetcher_B.wait();
    run_fetcher_C.wait();
    run_fetcher_D.wait();
    run_setup_aie.wait();
    if (duration != NULL)
      *duration += timer_execution.getElapsedSeconds();
  }

  //
  // Read the transformed floating volume from the board
  //
  void read_flt_transformed(double *duration = NULL) {
    Timer timer_transfer_read_flt;
    if (duration != NULL)
      timer_transfer_read_flt.start();
    buffer_setup_mi_flt_transformed.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    buffer_setup_mi_flt_transformed.read(output_flt);
    if (duration != NULL)
      *duration += timer_transfer_read_flt.getElapsedSeconds();
  }

  //
  // Read the mutual information from the board
  //
  float read_mutual_information() {
    // std::printf("no mutual information needs to be read from the board\n");
    float output_data = 42;
    buffer_mutual_info_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    buffer_mutual_info_output.read(&output_data);
    return output_data;
  }

  float hw_exec(float TX, float TY, float ANG, double *duration_exec = NULL) {
    set_transform_params(TX, TY, ANG);
    // std::cout << "Run completed - This will not save the output" <<
    // std::endl; std::cout << "Params used to save the volume: " << TX << " "
    // << TY << " " << ANG << std::endl;
    run(duration_exec);
    return read_mutual_information();
  }

  float hw_exec_tx(float TX, float TY, float ANG, double *duration_exec = NULL,
                   bool save = false) {
    set_transform_params(TX, TY, ANG);
    run(duration_exec);
    // std::cout << "Run completed" << std::endl;
    // std::cout << "Params used to save the volume: " << TX << " " << TY << " "
    // << ANG << std::endl;
    if (save) {
      // std::cout << "Saving the volume" << std::endl;
      read_flt_transformed();
    }
    return read_mutual_information();
  }

  ~Versal3DIR() {
    //delete[] input_ref;
    //delete[] input_flt;
    //delete[] output_flt;
  }
};
