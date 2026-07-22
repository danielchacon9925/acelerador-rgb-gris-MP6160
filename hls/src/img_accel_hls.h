#pragma once

void rgb_to_gray_hls(const unsigned char* input,
                     unsigned char* output,
                     unsigned int num_pixels);

void rgb_to_gray_top(const unsigned char* input,
                     unsigned char* output,
                     unsigned int num_pixels);
