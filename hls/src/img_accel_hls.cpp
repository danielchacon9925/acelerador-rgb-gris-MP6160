#include "img_accel_hls.h"

void rgb_to_gray_hls(const unsigned char* input,
                     unsigned char* output,
                     unsigned int num_pixels)
{
    for (unsigned int i = 0; i < num_pixels; ++i) {
        const unsigned int r = input[3 * i + 0];
        const unsigned int g = input[3 * i + 1];
        const unsigned int b = input[3 * i + 2];
        output[i] = static_cast<unsigned char>((77 * r + 150 * g + 29 * b) >> 8);
    }
}

void rgb_to_gray_top(const unsigned char* input,
                     unsigned char* output,
                     unsigned int num_pixels)
{
#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem_in
#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem_out
#pragma HLS INTERFACE s_axilite port=num_pixels bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS PIPELINE II=1
    rgb_to_gray_hls(input, output, num_pixels);
}
