
#ifndef _TOP_H_
#define _TOP_H_

#include "hls_video.h"
#include "stdint.h"

// maximum image size
#define MAX_WIDTH  1920
#define MAX_HEIGHT 1080

// I/O Image Settings
#define INPUT_IMAGE  "input_image.jpg"
#define OUTPUT_IMAGE "input_image_sobel.jpg"

#define ABSDIFF(x,y) ((x>y)? x - y : y - x)
#define ABS(x) ((x>0)? x : -x)

// typedef video library core structures
typedef hls::Scalar<2, unsigned char>                YUV_PIXEL;
typedef hls::Mat<MAX_HEIGHT, MAX_WIDTH, HLS_8UC2>    YUV_IMAGE;
typedef hls::Scalar<3, unsigned char>                RGB_PIXEL;
typedef hls::Mat<MAX_HEIGHT, MAX_WIDTH, HLS_8UC3>    RGB_IMAGE;
typedef hls::Window<3, 3, unsigned char>             Y_WINDOW;
typedef hls::LineBuffer<3, MAX_WIDTH, unsigned char> Y_BUFFER;

// top level function for HW synthesis
void sobel_filter(uint16_t* data_in,
                  uint16_t* data_out,
                  uint16_t* data_size_in,
                  uint16_t* data_size_out);

#endif
