#include <ap_int.h>
#include <hls_stream.h>
#include <hls_math.h>
#include <ap_axi_sdata.h>
#include "../common/common.h" 

#define IPE_PLIO_WIDTH_BITS 128 // TODO move in common.h

using fetcher_packet_t = ap_uint<NUM_PIXELS_PER_READ * 8>; // NUM_PIXELS_PER_READ one of {32, 64, 128} pixels, each pixel is 8 bits
using chunk_t = ap_uint<CHUNK_SIZE>; // 32 pixels (256 bits)
using double_chunk_t = ap_uint<2 * CHUNK_SIZE>; // 64 pixels (512 bits)
using aie_packet_t = ap_uint<IPE_PLIO_WIDTH_BITS>; // 16 pixels (plio is 128 bit)

#define NUM_CHUNKS_PER_FETCH (NUM_PIXELS_PER_READ / (CHUNK_SIZE / 8))
#define NUM_DOUBLE_CHUNKS_PER_INTERMEDIATE ( (2 * NUM_PIXELS_PER_READ * 8) / (2 * CHUNK_SIZE) )
#define TRIPCOUNT_DENOMINATOR (NUM_PIXELS_PER_READ)


void chunk_pair_interleave(
    hls::stream<fetcher_packet_t>& in_stream_1,
    hls::stream<fetcher_packet_t>& in_stream_2,
    hls::stream<double_chunk_t>& out_stream,
    const uint64_t num_iterations
);
void dispatcher(
    hls::stream<double_chunk_t>& in_stream_ab,
    hls::stream<double_chunk_t>& in_stream_cd,
    hls::stream<double_chunk_t> out_stream_ab[DS_PE],
    hls::stream<double_chunk_t> out_stream_cd[DS_PE],
    const uint64_t num_iterations,
    int n_couples
);
void data_scheduler(
    hls::stream<double_chunk_t>& in_stream_ab,
    hls::stream<double_chunk_t>& in_stream_cd,
    hls::stream<aie_packet_t> out_to_plio[INT_PE_SATURATED],
    const uint64_t num_iterations,
    int ds_id,
    int n_couples
);

void scheduler_IPE(
    hls::stream<fetcher_packet_t>& in_from_fetcher_a,
    hls::stream<fetcher_packet_t>& in_from_fetcher_b,
    hls::stream<fetcher_packet_t>& in_from_fetcher_c,
    hls::stream<fetcher_packet_t>& in_from_fetcher_d,
    int n_couples,
    hls::stream<aie_packet_t> out_to_plio[INT_PE_SATURATED]
) {
    #pragma HLS INTERFACE axis port=in_from_fetcher_a
    #pragma HLS INTERFACE axis port=in_from_fetcher_b
    #pragma HLS INTERFACE axis port=in_from_fetcher_c
    #pragma HLS INTERFACE axis port=in_from_fetcher_d
    #pragma HLS INTERFACE axis port=out_to_plio
    #pragma HLS INTERFACE s_axilite port=n_couples bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    #pragma HLS DATAFLOW

    const uint64_t NUM_ITERATIONS = (uint64_t)((DIMENSION * DIMENSION * n_couples) / NUM_PIXELS_PER_READ);

    hls::stream<double_chunk_t> stream_ab("stream_ab");
#pragma HLS STREAM variable = stream_ab depth = 4 * NUM_CHUNKS_PER_FETCH
    hls::stream<double_chunk_t> stream_cd("stream_cd");
#pragma HLS STREAM variable = stream_cd depth = 4 * NUM_CHUNKS_PER_FETCH

    hls::stream<double_chunk_t> stream_final_ab[DS_PE];
#pragma HLS STREAM variable = stream_final_ab depth = 1 * 16
#pragma HLS RESOURCE variable = stream_final_ab core = FIFO_BRAM

    hls::stream<double_chunk_t> stream_final_cd[DS_PE];
#pragma HLS STREAM variable = stream_final_cd depth = 1 * 16
#pragma HLS RESOURCE variable = stream_final_cd core = FIFO_BRAM

    chunk_pair_interleave(in_from_fetcher_a, in_from_fetcher_b, stream_ab, NUM_ITERATIONS); // genera: ...B2A2B1A1B0A0
    chunk_pair_interleave(in_from_fetcher_c, in_from_fetcher_d, stream_cd, NUM_ITERATIONS); // genera: ...D2C2D1C1D0C0

    dispatcher(stream_ab, stream_cd, stream_final_ab, stream_final_cd, NUM_ITERATIONS * NUM_CHUNKS_PER_FETCH, n_couples); // dispatch dei dati alle IPE
    
    // TODO potrebbe essere meglio passare solo un subset dell'array di stream ad ogni PE (però così funziona abbastanza bene comunque)
    for (int i = 0; i < DS_PE; i++) {
        #pragma HLS UNROLL
        data_scheduler(stream_final_ab[i], stream_final_cd[i], out_to_plio, NUM_ITERATIONS * NUM_CHUNKS_PER_FETCH, i, n_couples); // scheduling round robin che si adatta al mapping PLIO->IPE
    }
}

// Splitta la roba letta dalla memoria (32, 64 o 128 byte) in chunk da 32 byte
// Poi concatena i chunk a due a due (e.g. AB, CD) in pacchetti double_chunk
void chunk_pair_interleave(
    hls::stream<fetcher_packet_t>& in_stream_1,
    hls::stream<fetcher_packet_t>& in_stream_2,
    hls::stream<double_chunk_t>& out_stream,
    const uint64_t NUM_ITERATIONS
) {
    // commanti con << example >> mostrano le dimensioni dei pacchetti per NUM_PIXELS_PER_READ = 64

    cpi_main_loop: for (int i = 0; i < NUM_ITERATIONS; i++) {
        #pragma HLS PIPELINE II=1
        // #pragma HLS loop_tripcount min=DIMENSION*DIMENSION/TRIPCOUNT_DENOMINATOR max=DIMENSION*DIMENSION*N_COUPLES_MAX/TRIPCOUNT_DENOMINATOR avg=(DIMENSION*DIMENSION*4)/TRIPCOUNT_DENOMINATOR

        // Legge un pacchetto (NUM_PIXELS_PER_READ pixels) da ciascun fetcher
        fetcher_packet_t packet_1 = in_stream_1.read(); // << 512 bit A1A0 >>
        fetcher_packet_t packet_2 = in_stream_2.read(); // << 512 bit B1B0 >>

        // Splitta i pacchetti in chunk da 32 byte (8 pixels)
        chunk_t chunks_1[NUM_CHUNKS_PER_FETCH], chunks_2[NUM_CHUNKS_PER_FETCH]; // https://docs.amd.com/r/2023.1-English/ug1448-hls-guidance/Pipeline-Constraint-Violation
        #pragma HLS ARRAY_PARTITION variable=chunks_1 complete // https://docs.amd.com/r/2023.1-English/ug1399-vitis-hls/Array-Partitioning
        #pragma HLS ARRAY_PARTITION variable=chunks_2 complete

        for (int j = 0; j < NUM_CHUNKS_PER_FETCH; j++) {
            #pragma HLS UNROLL
            chunks_1[j] = packet_1.range(j*CHUNK_SIZE + CHUNK_SIZE-1, j*CHUNK_SIZE); // << j=0 -> chunks_1=[A0] ; j=1 -> chunks_1=[A1] >>
            chunks_2[j] = packet_2.range(j*CHUNK_SIZE + CHUNK_SIZE-1, j*CHUNK_SIZE); // << j=0 -> chunks_2=[B0] ; j=1 -> chunks_2=[B1] >>
        }
        
        double_chunk_t out_packet[NUM_CHUNKS_PER_FETCH];
        // Fa interleave dei chunk da due fetcher a due a due
        for (int j = 0; j < NUM_CHUNKS_PER_FETCH; j++) {
            #pragma HLS UNROLL
            // Concatena i chunk in ordine nel double_chunk (i.e. A0B0, A1B1, A2B2, ...)
            out_packet[j].range(CHUNK_SIZE - 1, 0) = chunks_1[j]; // TODO ottimizzare: forse non serve nemmeno avere "chunks_1" e "chunks_2", si può direttamente usare il range
            out_packet[j].range(2 * CHUNK_SIZE - 1, CHUNK_SIZE) = chunks_2[j];
        }

        for (int j = 0; j < NUM_CHUNKS_PER_FETCH; j++) {
            #pragma HLS UNROLL
            out_stream.write(out_packet[j]);
        }
    }
}

// Invia i pacchetti double_chunk alle varie scheduling PE (funzione data_schaduler)
void dispatcher(
    hls::stream<double_chunk_t>& in_stream_ab,
    hls::stream<double_chunk_t>& in_stream_cd,
    hls::stream<double_chunk_t> out_stream_ab[DS_PE],
    hls::stream<double_chunk_t> out_stream_cd[DS_PE],
    const uint64_t NUM_ITERATIONS,
    int n_couples
) {
    // Dispatch dei dati alle IPE
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        #pragma HLS PIPELINE II=1

        double_chunk_t final_packet_ab = in_stream_ab.read(); // 64 pixels (512 bits)
        double_chunk_t final_packet_cd = in_stream_cd.read(); // 64 pixels (512 bits)

        // Invia i pacchetti alle scheduling PE
        out_stream_ab[(i >> INT_PE_PER_DS_EXPO) & DS_PE_MASK].write(final_packet_ab);
        out_stream_cd[(i >> INT_PE_PER_DS_EXPO) & DS_PE_MASK].write(final_packet_cd);
    }
}


// Invia le sequenze DCBA alle IPE con round robin.
// Per INT_PE = 128, dove si hanno due IPE per PLIO, adatta lo scheduling al mapping 1:2 PLIO->IPE
void data_scheduler(
    hls::stream<double_chunk_t>& in_stream_ab,
    hls::stream<double_chunk_t>& in_stream_cd,
    hls::stream<aie_packet_t> out_to_plio[INT_PE_SATURATED], // TODO non ha senso avere INT_PE_SATURATED, ma INT_PE_PER_DS
    const uint64_t NUM_ITERATIONS,
    int ds_id, // ID del data scheduler
    int n_couples
) {
    const int start_int_pe = ds_id * INT_PE_PER_DS; // Indice di partenza per il Data Scheduler corrente
    const int end_int_pe = (ds_id + 1) * INT_PE_PER_DS; // Indice di fine per il Data Scheduler corrente

    header_generation_loop: for (int i = start_int_pe; i < end_int_pe; ++i) {
        #pragma HLS PIPELINE II=16

        #if INT_PE == 2 * MAX_INT_PE_PLIOS
            // caso 128 PE: ogni PLIO serve due PE logiche (2*i e 2*i+1).

            // header per IPE pari: 0, 2, 4...
            aie_packet_t header_even_c1 = 0;
            header_even_c1.range(31, 0)   = i;
            header_even_c1.range(63, 32)  = n_couples;
            out_to_plio[i >> 1].write(header_even_c1);
            for (int j = 0; j < 7; j++) { // 7 chunk di padding
                #pragma HLS UNROLL
                out_to_plio[i >> 1].write(0); 
            }

        #elif (INT_PE >= 1) && (INT_PE <= MAX_INT_PE_PLIOS)
            aie_packet_t header_c1 = 0;
            header_c1.range(31, 0)   = i;
            header_c1.range(63, 32)  = n_couples;
            out_to_plio[i].write(header_c1);
            for (int j = 0; j < 7; j++) { // 7 chunk di padding
                #pragma HLS UNROLL
                out_to_plio[i].write(0);
            }
        #else
            #error "INT_PE must be a power of two between 1 and 2*MAX_INT_PE_PLIOS"
        #endif
    }


    #if INT_PE > 1
    ap_uint<INT_PE_PER_DS_EXPO> pe_idx = 0; // Importante che il numero di bit sia corretto: il contatore si resetta da solo (richiede che le IPE siano una potenza di due)
    #endif

    // --- main data scheduling loop ---

    ds_main_loop: for (int i = 0; i < NUM_ITERATIONS / DS_PE; i++) {
        #pragma HLS PIPELINE II=8
        // #pragma HLS loop_tripcount min=DIMENSION*DIMENSION/TRIPCOUNT_DENOMINATOR max=DIMENSION*DIMENSION*N_COUPLES_MAX/TRIPCOUNT_DENOMINATOR avg=(DIMENSION*DIMENSION*4)/TRIPCOUNT_DENOMINATOR

        double_chunk_t final_packet_ab_0 = in_stream_ab.read(); // 64 pixels (512 bits)
        double_chunk_t final_packet_cd_0 = in_stream_cd.read(); // 64 pixels (512 bits)

        // Nota:
        // - final_packet_ab_0 ha 64 pixels, che vanno divisi in 4 chunk da 16 pixels (128 bits): B[31,16],B[15,0],A[31,16],A[15,0]
        // - stessa cosa per final_packet_cd_0: D[31,16],D[15,0],C[31,16],C[15,0]

        #if INT_PE == 1 
            // Invia pacchetto alla IPE pari e dispari nella stessa plio
            dispatch_loop_1IPE_AABB:
            for (int j = 0; j < 4; j++) {
                #pragma HLS UNROLL
                out_to_plio[0].write(final_packet_ab_0.range(j*IPE_PLIO_WIDTH_BITS + IPE_PLIO_WIDTH_BITS-1, j*IPE_PLIO_WIDTH_BITS));
            }
            dispatch_loop_1IPE_CCDD:
            for (int j = 0; j < 4; j++) {
                #pragma HLS UNROLL
                out_to_plio[0].write(final_packet_cd_0.range(j*IPE_PLIO_WIDTH_BITS + IPE_PLIO_WIDTH_BITS-1, j*IPE_PLIO_WIDTH_BITS));
            }

        #elif INT_PE == 2 * MAX_INT_PE_PLIOS
            const uint32_t plio_index = (pe_idx >> 1) + (start_int_pe >> 1); // Si incrementa ogni due iterazioni
            
            // --- sending packets to PLIOs ---

            dispatch_loop_128PIEs_AABB:
            for (int j = 0; j < 4; j++) {
                #pragma HLS UNROLL
                out_to_plio[plio_index].write(final_packet_ab_0.range(j*IPE_PLIO_WIDTH_BITS + IPE_PLIO_WIDTH_BITS-1, j*IPE_PLIO_WIDTH_BITS));
            }
            dispatch_loop_128PIEs_CCDD:
            for (int j = 0; j < 4; j++) {
                #pragma HLS UNROLL
                out_to_plio[plio_index].write(final_packet_cd_0.range(j*IPE_PLIO_WIDTH_BITS + IPE_PLIO_WIDTH_BITS-1, j*IPE_PLIO_WIDTH_BITS));
            }

        #elif (INT_PE > 1) && (INT_PE <= MAX_INT_PE_PLIOS)
            const uint32_t plio_index = pe_idx + start_int_pe;
            
            // --- sending packets to PLIOs ---

            dispatch_loop_default_AABB: for (int j = 0; j < 4; j++) {
                out_to_plio[plio_index].write(final_packet_ab_0.range(j*IPE_PLIO_WIDTH_BITS + IPE_PLIO_WIDTH_BITS-1, j*IPE_PLIO_WIDTH_BITS));
            }
            dispatch_loop_default_CCDD: for (int j = 0; j < 4; j++) {
                out_to_plio[plio_index].write(final_packet_cd_0.range(j*IPE_PLIO_WIDTH_BITS + IPE_PLIO_WIDTH_BITS-1, j*IPE_PLIO_WIDTH_BITS));
            }
        #else
            #error "INT_PE must be a power of two between 1 and 2*MAX_INT_PE_PLIOS"
        #endif

        pe_idx += 1; // round robin sulle plio
    }
}
