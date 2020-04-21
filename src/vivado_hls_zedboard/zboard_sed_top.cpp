
#include "zboard_sed_top.h"
#include "ap_int.h"
#include <cstring>

//Sobel Computation using a 3x3 neighborhood
YUV_PIXEL sobel_operator(Y_WINDOW *window,
                         int XR0C0, int XR0C1, int XR0C2,
                         int XR1C0, int XR1C1, int XR1C2,
                         int XR2C0, int XR2C1, int XR2C2,
                         int YR0C0, int YR0C1, int YR0C2,
                         int YR1C0, int YR1C1, int YR1C2,
                         int YR2C0, int YR2C1, int YR2C2,
                         int high_thresh, int low_thresh, int invert)
{
    short x_weight = 0;
    short y_weight = 0;

    short edge_weight;
    unsigned char edge_val;
    YUV_PIXEL pixel;

    char i;
    char j;

    const char x_op[3][3] = {{XR0C0,XR0C1,XR0C2},
                             {XR1C0,XR1C1,XR1C2},
                             {XR2C0,XR2C1,XR2C2}};

    const char y_op[3][3] = {{YR0C0,YR0C1,YR0C2},
                             {YR1C0,YR1C1,YR1C2},
                             {YR2C0,YR2C1,YR2C2}};


    //Compute approximation of the gradients in the X-Y direction
    for(i=0; i < 3; i++)
    {
        for(j = 0; j < 3; j++)
        {
            // X direction gradient
            x_weight = x_weight + (window->getval(i,j) * x_op[i][j]);

            // Y direction gradient
            y_weight = y_weight + (window->getval(i,j) * y_op[i][j]);
        }
    }

    edge_weight = ABS(x_weight) + ABS(y_weight);

    if (edge_weight < 255)
    {
        edge_val = (255-(unsigned char)(edge_weight));
    }
    else
    {
        edge_val = 0;
    }

    //Edge thresholding
    if(edge_val > high_thresh)
    {
        edge_val = 255;
    }
    else if(edge_val < low_thresh)
    {
        edge_val = 0;
    }

    // Invert
    if (invert == 1)
    {
        edge_val = 255 - edge_val;
    }

    pixel.val[0] = edge_val;
    pixel.val[1] = 128;

    return pixel;
}


void sobel_filter_core(YUV_IMAGE& src, YUV_IMAGE& dst, int rows, int cols,
                       int C_XR0C0, int C_XR0C1, int C_XR0C2,
                       int C_XR1C0, int C_XR1C1, int C_XR1C2,
                       int C_XR2C0, int C_XR2C1, int C_XR2C2,
                       int C_YR0C0, int C_YR0C1, int C_YR0C2,
                       int C_YR1C0, int C_YR1C1, int C_YR1C2,
                       int C_YR2C0, int C_YR2C1, int C_YR2C2,
                       int c_high_thresh, int c_low_thresh, int c_invert)
{
    Y_BUFFER buff_A;
    Y_WINDOW buff_C;

    for(int row = 0; row < rows+1; row++)
    {
        for(int col = 0; col < cols+1; col++)
        {
            #pragma HLS loop_flatten off
            #pragma HLS dependence variable=&buff_A false
            #pragma HLS PIPELINE II = 1

            // Temp values are used to reduce the number of memory reads
            unsigned char temp;
            YUV_PIXEL tempx;

            //Line Buffer fill
            if(col < cols)
            {
                buff_A.shift_down(col);
                temp = buff_A.getval(0,col);
            }

            //There is an offset to accomodate the active pixel region
            //There are only MAX_WIDTH and MAX_HEIGHT valid pixels in the image
            if(col < cols && row < rows)
            {
                YUV_PIXEL new_pix;
                src >> new_pix;
                tempx = new_pix;
                buff_A.insert_bottom(tempx.val[0],col);
            }

            //Shift the processing window to make room for the new column
            buff_C.shift_right();

            //The Sobel processing window only needs to store luminance values
            //rgb2y function computes the luminance from the color pixel
            if(col < cols)
            {
                buff_C.insert(buff_A.getval(2,col),2,0);
                buff_C.insert(temp,1,0);
                buff_C.insert(tempx.val[0],0,0);
            }
            YUV_PIXEL edge;

            //The sobel operator only works on the inner part of the image
            //This design assumes there are no edges on the boundary of the image
            if( row <= 1 || col <= 1 || row > (rows-1) || col > (cols-1))
            {
                edge.val[0] = 0;
                edge.val[1] = 128;
            }
            else
            {
                //Sobel operation on the inner portion of the image
                edge = sobel_operator(&buff_C,
                                      C_XR0C0, C_XR0C1, C_XR0C2, C_XR1C0, C_XR1C1, C_XR1C2, C_XR2C0, C_XR2C1, C_XR2C2,
                                      C_YR0C0, C_YR0C1, C_YR0C2, C_YR1C0, C_YR1C1, C_YR1C2, C_YR2C0, C_YR2C1, C_XR2C2,
                                      c_high_thresh, c_low_thresh, c_invert);
            }

            //The output image is offset from the input to account for the line buffer
            if(row > 0 && col > 0)
            {
                dst << edge;
            }
        }
    }
}


void fill_data(uint16_t* data_size,
               uint16_t rows,
               uint16_t cols)
{
    *data_size++ = rows;
    *data_size++ = cols;
}


void sobel_filter(uint16_t* data_in,
                  uint16_t* data_out,
                  uint16_t* data_size_in,
                  uint16_t* data_size_out)
{
    //Create AXI streaming interfaces for the core
    #pragma HLS INTERFACE axis port=data_in       bundle=in_data
    #pragma HLS INTERFACE axis port=data_out      bundle=out_data
    #pragma HLS INTERFACE axis port=data_size_in  bundle=in_data_size
    #pragma HLS INTERFACE axis port=data_size_out bundle=out_data_size
    #pragma HLS INTERFACE ap_ctrl_none port=return

    // Predefined data - Sobel Filter Kernel
    int C_XR0C0 =  1; int C_XR0C1 =  0; int C_XR0C2 = -1;
    int C_XR1C0 =  2; int C_XR1C1 =  0; int C_XR1C2 = -2;
    int C_XR2C0 =  1; int C_XR2C1 =  0; int C_XR2C2 = -1;
    int C_YR0C0 =  1; int C_YR0C1 =  2; int C_YR0C2 =  1;
    int C_YR1C0 =  0; int C_YR1C1 =  0; int C_YR1C2 =  0;
    int C_YR2C0 = -1; int C_YR2C1 = -2; int C_YR2C2 = -1;
    int c_high_thresh = 250;
    int c_low_thresh = 50;
    int c_invert = 1;

    uint16_t rows = *data_size_in++;
    uint16_t cols = *data_size_in++;

    YUV_IMAGE img_src_yuv(rows, cols);
    YUV_IMAGE img_dst_yuv(rows, cols);

    hls::Array2Mat<MAX_WIDTH, uint16_t, MAX_HEIGHT, MAX_WIDTH, HLS_8UC2>
                  (data_in, img_src_yuv);

    #pragma HLS DATAFLOW
    sobel_filter_core(img_src_yuv, img_dst_yuv, rows, cols,
                      C_XR0C0, C_XR0C1, C_XR0C2,
                      C_XR1C0, C_XR1C1, C_XR1C2,
                      C_XR2C0, C_XR2C1, C_XR2C2,
                      C_YR0C0, C_YR0C1, C_YR0C2,
                      C_YR1C0, C_YR1C1, C_YR1C2,
                      C_YR2C0, C_YR2C1, C_YR2C2,
                      c_high_thresh, c_low_thresh, c_invert);


    fill_data(data_size_out, rows, cols);

    hls::Mat2Array<MAX_WIDTH, uint16_t, MAX_HEIGHT, MAX_WIDTH, HLS_8UC2>
                  (img_dst_yuv, data_out);

}
