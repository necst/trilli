#include "common.h"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"
#include "kernel_out_pixels_merger.h"

void out_pixels_merger(input_stream<uint8>* restrict in0, input_stream<uint8>* restrict in1, output_stream<uint8>* restrict out)
{
    // reading n_couples
    uint8 n_couples_bytes[4];
    n_couples_bytes[0] = readincr(in0);
    n_couples_bytes[1] = readincr(in0);
    n_couples_bytes[2] = readincr(in0);
    n_couples_bytes[3] = readincr(in0);
    int n_couples = ((uint32)n_couples_bytes[0]) + (((uint32)n_couples_bytes[1]) << 8) + (((uint32)n_couples_bytes[2]) << 16) + (((uint32)n_couples_bytes[3]) << 24);

    printf("merger: n_couples=%d\n", n_couples);

    for (int i = 0; i < (DIMENSION * DIMENSION * n_couples / INT_PE) / 32; i++)
        chess_loop_range(DIMENSION*DIMENSION/INT_PE,) // TODO check again if it's correct
        chess_prepare_for_pipelining
    {
        aie::vector<uint8, 32> vector_0 = readincr_v<32>(in0);
        aie::vector<uint8, 32> vector_1 = readincr_v<32>(in1);

        writeincr(out, vector_0);
        writeincr(out, vector_1);
    }
}
