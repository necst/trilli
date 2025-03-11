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

#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <ap_axi_sdata.h>
#include "../../../mutual_info/include/hw/mutualInfo/mutual_info.hpp"
#include "../../../mutual_info/mutual_information_master.cpp"
#include "../../../sw/include/image_utils/image_utils.cpp"
#include "../../fetcher_A.cpp"
#include "../../fetcher_B.cpp"
#include "../../fetcher_C.cpp"
#include "../../fetcher_D.cpp"
#include "../../setup_interpolator.cpp"
#include "../../writer.cpp"
#include "../../setup_aie.cpp"
#include "../../setup_mi.cpp"
#include "../../../sw/include/image_utils/image_utils.hpp"
#include "../../../sw/include/software_mi/software_mi.hpp"
#include <cmath>
#include <string>

#include "../utils.hpp"

#define DEFAULT_TX 18.54458648
#define DEFAULT_TY -12.30391042
#define DEFAULT_ANG_DEG 20

#define AIE_PATH "../../../aie/"
#define SW_PATH "../../../sw/"
#define TEST_PATH "../"

#define AIE_FOLDER(x) (AIE_PATH x)
#define SW_FOLDER(x) (SW_PATH x)
#define TEST_FOLDER(x) (TEST_PATH x)

#define COORD_AXIS_W COORD_BITWIDTH, 0, 0, 0

typedef ap_uint<8> ORIGINAL_PIXEL_TYPE;
typedef ap_uint<INPUT_DATA_BITWIDTH_FETCHER> WIDE_PIXEL_TYPE;
typedef ap_uint<INPUT_DATA_BITWIDTH_FETCHER_MIN> AIE_PIXEL_TYPE;
typedef ap_uint<INPUT_DATA_BITWIDTH> MI_PIXEL_TYPE;
typedef ap_axis<COORD_AXIS_W> COORDS_TYPE;

void run_aie() {
    std::string command = std::string("make -C ") + AIE_PATH + " aie_simulate_x86";
    std::cout << std::endl << std::flush;
    system(command.c_str());
    std::cout << "Done" << std::endl << std::endl;
}

void create_folder(const std::string& folder) {
    std::string command = std::string("mkdir -p ") + folder;
    system(command.c_str());
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 5) {
        std::cerr << "usage: ./testbench_ultimate <n_couples> [<TX> <TY> <ANG_DEG>]" << std::endl;
        return -1;
    }

    const int n_couples = atoi(argv[1]);
    const int padding = (NUM_PIXELS_PER_READ - (n_couples % NUM_PIXELS_PER_READ)) % NUM_PIXELS_PER_READ;

    const float TX = (argc == 5) ? atof(argv[2]) : DEFAULT_TX;
    const float TY = (argc == 5) ? atof(argv[3]) : DEFAULT_TY;
    const float ANG_DEG = (argc == 5) ? atof(argv[4]) : DEFAULT_ANG_DEG;
    const float ANG = (ANG_DEG * M_PI) / 180.f; // radians

    std::cout << "TESTBENCH PARAMETERS:\n";
    std::cout << "n_couples: " << n_couples << std::endl;
    std::cout << "padding: " << padding << std::endl;
    std::cout << "TX: " << TX << std::endl;
    std::cout << "TY: " << TY << std::endl;
    std::cout << "ANG_DEG: " << ANG_DEG << std::endl;
    std::cout << "ANG: " << ANG << std::endl;
    std::cout << "------------------------------\n";

    uint8_t* input_volume     = new uint8_t[DIMENSION*DIMENSION * (n_couples + padding)];
    uint8_t* output_volume_hw = new uint8_t[DIMENSION*DIMENSION * (n_couples + padding)];
    uint8_t* output_volume_sw = new uint8_t[DIMENSION*DIMENSION * (n_couples + padding)];

    read_volume_from_file(input_volume, DIMENSION, n_couples, 0, padding, SW_FOLDER("dataset/"));



    //
    // ---------- (1) setup aie ---------- 
    //
    std::printf("-> Running setup_aie\n");
    hls::stream<float> out_setup_aie("out_setup_aie");
    setup_aie(TX, TY, ANG, n_couples + padding, out_setup_aie);
    write_stream_to_file(out_setup_aie, AIE_FOLDER("data/params_in.txt"), PLIO_32);

    // write fake streams to file
    {
        std::cout << "-> Writing fake streams to file ..." << std::endl;
        hls::stream<ORIGINAL_PIXEL_TYPE> fake_ab[INT_PE];
        hls::stream<ORIGINAL_PIXEL_TYPE> fake_cd[INT_PE];

        for (int i = 0; i < INT_PE; i++) {
            fake_ab[i].write(i);
            fake_cd[i].write(i);
            for (int j = 0; j < 64-1; j++) {
                fake_ab[i].write(0);
                fake_cd[i].write(0);
            }
            for (int j = 0; j <  2 * DIMENSION * DIMENSION * (n_couples + padding) / INT_PE; j++) {
                fake_ab[i].write(0);
                fake_cd[i].write(0);
            }
        }

        for (int i = 0; i < INT_PE; i++) {
            write_stream_to_file(fake_ab[i], AIE_FOLDER("data/p_ab_" + std::to_string(i+1) + ".txt"), PLIO_128);
            write_stream_to_file(fake_cd[i], AIE_FOLDER("data/p_cd_" + std::to_string(i+1) + ".txt"), PLIO_128);
        }
    }

    //
    // ---------- (2) AIE ----------
    //
    std::printf("-> Running AIE (indexes & coefficients) . . .\n");
    run_aie();
    hls::stream<COORDS_TYPE> out_aie_A("out_aie_A");
    hls::stream<COORDS_TYPE> out_aie_B("out_aie_B");
    hls::stream<COORDS_TYPE> out_aie_C("out_aie_C");
    hls::stream<COORDS_TYPE> out_aie_D("out_aie_D");
    read_stream_from_file<COORD_AXIS_W>(out_aie_A, AIE_FOLDER("x86simulator_output/data/TL_out.txt"));
    read_stream_from_file<COORD_AXIS_W>(out_aie_B, AIE_FOLDER("x86simulator_output/data/TR_out.txt"));
    read_stream_from_file<COORD_AXIS_W>(out_aie_C, AIE_FOLDER("x86simulator_output/data/BL_out.txt"));
    read_stream_from_file<COORD_AXIS_W>(out_aie_D, AIE_FOLDER("x86simulator_output/data/BR_out.txt"));

    //
    // ---------- (3) SETUP MUTUAL INFO ----------
    //
    std::printf("-> Running fetchers\n");
    hls::stream<WIDE_PIXEL_TYPE> out_fetcher_A("out_setup_mutualInfo_A");
    hls::stream<WIDE_PIXEL_TYPE> out_fetcher_B("out_setup_mutualInfo_B");
    hls::stream<WIDE_PIXEL_TYPE> out_fetcher_C("out_setup_mutualInfo_C");
    hls::stream<WIDE_PIXEL_TYPE> out_fetcher_D("out_setup_mutualInfo_D");
    fetcher_A(out_aie_A, out_fetcher_A, (WIDE_PIXEL_TYPE*) input_volume, n_couples + padding);
    fetcher_B(out_aie_B, out_fetcher_B, (WIDE_PIXEL_TYPE*) input_volume, n_couples + padding);
    fetcher_C(out_aie_C, out_fetcher_C, (WIDE_PIXEL_TYPE*) input_volume, n_couples + padding);
    fetcher_D(out_aie_D, out_fetcher_D, (WIDE_PIXEL_TYPE*) input_volume, n_couples + padding);

    //
    // ---------- (4) SETUP INTERPOLATOR ----------
    //
    std::printf("-> Running setup_interpolator\n");
    hls::stream<ap_uint<INPUT_DATA_BITWIDTH_FETCHER_MIN>> out_interpolator_ab[INT_PE];
    hls::stream<ap_uint<INPUT_DATA_BITWIDTH_FETCHER_MIN>> out_interpolator_cd[INT_PE];

    setup_interpolator(
        out_fetcher_A, out_fetcher_B, 
        SETUP_INTERPOLATOR_TESTBENCH_CALL(out_interpolator_ab),
        n_couples + padding
    );
    setup_interpolator(
        out_fetcher_C, out_fetcher_D, 
        SETUP_INTERPOLATOR_TESTBENCH_CALL(out_interpolator_cd),
        n_couples + padding);
    
    for (int i = 0; i < INT_PE; i++) {
        write_stream_to_file_unpack<ap_uint<INPUT_DATA_BITWIDTH_FETCHER_MIN>, ORIGINAL_PIXEL_TYPE>(out_interpolator_ab[i], AIE_FOLDER("data/p_ab_" + std::to_string(i+1) + ".txt"), PLIO_128);
        write_stream_to_file_unpack<ap_uint<INPUT_DATA_BITWIDTH_FETCHER_MIN>, ORIGINAL_PIXEL_TYPE>(out_interpolator_cd[i], AIE_FOLDER("data/p_cd_" + std::to_string(i+1) + ".txt"), PLIO_128);
    }

    //
    // ---------- (5) AIE interpolator ----------
    //
    std::printf("-> Running AIE (interpolator) . . .\n");
    run_aie();
    hls::stream<AIE_PIXEL_TYPE> out_aie_interpolated[INT_PE];
    for (int i = 0; i < INT_PE; i++) {
        read_stream_from_file_pack<ORIGINAL_PIXEL_TYPE, AIE_PIXEL_TYPE>(out_aie_interpolated[i], AIE_FOLDER("x86simulator_output/data/result_" + std::to_string(i+1) + ".txt"));
    }

    //
    // ---------- (6) writer ----------
    //
    // std::printf("-> Running writer\n");
    // writer(
    //     WRITER_TESTBENCH_CALL(out_aie_interpolated),
    //     (WIDE_PIXEL_TYPE*)output_volume_hw, 
    //     n_couples+padding);

    //
    // ---------- (6) setup_mi ----------
    
    std::printf("-> Running setup_mi\n");
    hls::stream<MI_PIXEL_TYPE> out_setup_mi("out_setup_mi");
    setup_mi(
        SETUP_MI_TESTBENCH_CALL(out_aie_interpolated), 
        out_setup_mi,
        (WIDE_PIXEL_TYPE*) output_volume_hw,
        n_couples + padding
    );

    // Salva out_setup_mi in numeri da 1 byte su file
    // write_stream_to_file_unpack<MI_PIXEL_TYPE, ORIGINAL_PIXEL_TYPE>(out_setup_mi, "mi_in.txt" , PLIO_32);

    //write_stream_to_file_(out_setup_mi, "mi_in.txt", PLIO_32);
    // ---------- (7) mutual_info ----------
    //
    // std::cout << "SIZE: " << out_setup_mi.size() << std::endl;
    // return 0;
    printf("Size of stream before: %d\n", out_setup_mi.size());



    std::printf("-> Running mutual_info\n");
    float hw_mi;

    //NOTA: Io credo che vada comunque passato n_couples + padding.... ma non sono sicuro. Non cambia nulla ad ora in questo test
    
    mutual_information_master(out_setup_mi, (MI_PIXEL_TYPE*) input_volume, &hw_mi, n_couples + padding, padding);
    printf("Size of stream after: %d\n", out_setup_mi.size());

    //write_stream_to_file_unpack<MI_PIXEL_TYPE, ORIGINAL_PIXEL_TYPE>(out_setup_mi, "leftover.txt" , PLIO_32);


    // write volume to file
    std::printf("-> Writing volume to file\n");
    create_folder(TEST_FOLDER("dataset_output_new/"));
    write_volume_to_file(output_volume_hw, DIMENSION, n_couples, 0, padding, TEST_FOLDER("dataset_output_new/"));

    std::cout << "--- TESTBENCH HW COMPLETED ---" << std::endl;
    return 0;
    // ###############################################################################################################################

    //
    // ---------- (7) SW transform ----------
    //
    std::printf("-> Running SW transform\n");
    transform_volume(input_volume, output_volume_sw, TX, TY, ANG, DIMENSION, n_couples + padding, true);
    std::printf("Writing volume to file ...\n");
    create_folder(TEST_FOLDER("dataset_output_sw/"));
    write_volume_to_file(output_volume_sw, DIMENSION, n_couples, 0, padding, TEST_FOLDER("dataset_output_sw/"));
    
    printf("-> Running SW mutual_info\n");
    float sw_mi = software_mi(n_couples + padding, TX, TY, ANG, SW_FOLDER("dataset/"), nullptr);

    std::cout << "--- TESTBENCH SW COMPLETED ---" << std::endl;


    std::cout << "remaining pixels: " << out_aie_interpolated[7].size() << std::endl;
    
    float error = std::abs(hw_mi - sw_mi);
    std::cout << "HW MI: " << hw_mi << std::endl;
    std::cout << "SW MI: " << sw_mi << std::endl;
    std::cout << "Error: " << error << std::endl;

    return 0;
}
