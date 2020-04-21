
#include "top.h"
#include "hls_opencv.h"
#include "stdint.h"
#include <cstring>
#include <iostream>

using namespace std;


// Definitions
void initDataPacket(uint16_t* sdata)
{
    memset(sdata, 0, sizeof(sdata));
}


void cvtcolor_rgb2yuv422(cv::Mat& rgb, cv::Mat& yuv)
{
    cv::Mat yuv444(rgb.rows, rgb.cols, CV_8UC3);
    cv::cvtColor(rgb, yuv444, CV_BGR2YUV);

    // chroma subsampling: yuv444 -> yuv422;
    for (int row = 0; row < yuv444.rows; row++)
    {
        for (int col = 0; col < yuv444.cols; col+=2)
        {
            cv::Vec3b p0_in = yuv444.at<cv::Vec3b>(row, col);
            cv::Vec3b p1_in = yuv444.at<cv::Vec3b>(row, col+1);
            cv::Vec2b p0_out, p1_out;
            p0_out.val[0] = p0_in.val[0];
            p0_out.val[1] = p0_in.val[1];
            p1_out.val[0] = p1_in.val[0];
            p1_out.val[1] = p0_in.val[2];
            yuv.at<cv::Vec2b>(row, col) = p0_out;
            yuv.at<cv::Vec2b>(row, col+1) = p1_out;
        }
    }
}


int main (int argc, char** argv)
{
    uint16_t* sdata_in       = new uint16_t[MAX_HEIGHT * MAX_WIDTH];
    uint16_t* sdata_out      = new uint16_t[MAX_HEIGHT * MAX_WIDTH];
    uint16_t* sdata_size_in  = new uint16_t[2];
    uint16_t* sdata_size_out = new uint16_t[2];

    initDataPacket(sdata_in);
    initDataPacket(sdata_out);

    cv::Mat src_rgb = cv::imread(INPUT_IMAGE);
    if (!src_rgb.data)
    {
        std::cout << "ERROR: could not open or find the input image!" << endl;
        return -1;
    }

    uint16_t rows = src_rgb.rows;
    uint16_t cols = src_rgb.cols;

    cv::Mat src_yuv(rows, cols, CV_8UC2);
    cv::Mat dst_yuv(rows, cols, CV_8UC2);
    cv::Mat dst_rgb(rows, cols, CV_8UC3);

    cvtcolor_rgb2yuv422(src_rgb, src_yuv);

    // Converting image data into bytestream
    uint32_t img_size = rows * cols;
    if (img_size > MAX_WIDTH * MAX_HEIGHT)
    {
        std::cout << "ERROR: Image size (" << img_size << ") bigger than 1920x1080." << endl;
        return -1;
    }

    sdata_size_in[0] = rows;
    sdata_size_in[1] = cols;
    memcpy(sdata_in, src_yuv.data, img_size * sizeof(uint16_t));

    int64 t0 = cv::getTickCount();
    sobel_filter(sdata_in,
                 sdata_out,
                 sdata_size_in,
                 sdata_size_out);
    int64 t1 = cv::getTickCount();
    double secs = (t1-t0)/cv::getTickFrequency();

    rows = sdata_size_out[0];
    cols = sdata_size_out[1];
    dst_yuv = cv::Mat(rows, cols, CV_8UC2, sdata_out).clone();
    cv::cvtColor(dst_yuv, dst_rgb, CV_YUV2BGR_YUYV);
    cv::imwrite(OUTPUT_IMAGE, dst_rgb);

    std::cout << "Sobel Compute Time: " << secs << std::endl;

    return 0;
}
