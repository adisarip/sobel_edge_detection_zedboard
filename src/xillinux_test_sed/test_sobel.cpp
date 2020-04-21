
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/types_c.h"
#include "stdint.h"
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

using namespace std;

#define MAX_HEIGHT   1080 // max rows
#define MAX_WIDTH    1920 // max columns
#define INPUT_IMAGE  "input_image.jpg"
#define OUTPUT_IMAGE "input_image_sobel.jpg"


// Definitions
void initDataPacket(uint16_t* sdata)
{
    size_t sdata_size = MAX_HEIGHT * MAX_WIDTH;
    memset(sdata, 0, sdata_size);
}


void all_write(int fd, uint16_t* buf, long int len)
{
    long int sent = 0;
    long int rc;

    while (sent < len)
    {
        rc = write(fd, buf + sent, len - sent);

        if ((rc < 0) && (errno == EINTR))
        {
            continue;
        }
        if (rc < 0)
        {
            std::cerr << "allwrite() failed to write" << std::endl;
            exit(1);
        }
        if (rc == 0)
        {
            std::cerr << "Reached write EOF (?!)" << endl;
            exit(1);
        }

        sent += rc;
    }
}


void all_read(int fd, uint16_t* buf, long int len)
{
    long int received = 0;
    long int rc;

    while (received < len)
    {
        rc = read(fd, buf + received, len-received);

        if ((rc < 0) && (errno == EINTR))
        {
            continue;
        }
        if (rc < 0)
        {
            std::cerr << "allread() failed to read" << endl;
            exit(1);
        }
        if (rc == 0)
        {
            std::cerr << "Reached read EOF (?!)\n" << endl;
            exit(1);
        }

        received += rc;
    }
}


void cvtcolor_rgb2yuv422(cv::Mat& rgb, cv::Mat& yuv)
{
    cv::Mat yuv444(rgb.rows, rgb.cols, CV_8UC3);
    cv::cvtColor(rgb, yuv444, CV_BGR2YUV); // CV_BGR2YUV = 84

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
    int fdw1, fdw2, fdr1, fdr2;
    pid_t pid;

    // Read the input image
    cv::Mat src_rgb = cv::imread(INPUT_IMAGE, cv::IMREAD_COLOR);
    if (!src_rgb.data)
    {
        printf("ERROR: could not open or find the input image!\n");
        return -1;
    }

    uint16_t rows = src_rgb.rows;
    uint16_t cols = src_rgb.cols;
    uint32_t img_size = rows * cols;

    if (img_size > MAX_WIDTH * MAX_HEIGHT)
    {
        printf("ERROR: Image size (%u) bigger than 1920x1080.\n", img_size);
        return -1;
    }

    // Open all the stream device files
    fdw1 = open("/dev/xillybus_write_16_1", O_WRONLY);
    if (fdw1 < 0)
    {
        std::cout << "Failed to open xillybus_write_16_1 device file" << endl;
        return -1;
    }

    fdw2 = open("/dev/xillybus_write_16_2", O_WRONLY);
    if (fdw2 < 0)
    {
        std::cout << "Failed to open xillybus_write_16_2 device file" << endl;
        return -1;
    }

    fdr1 = open("/dev/xillybus_read_16_1", O_RDONLY);
    if (fdr1 < 0)
    {
        std::cout << "Failed to open xillybus_read_16_1 device file" << endl;
        return -1;
    }

    fdr2 = open("/dev/xillybus_read_16_2", O_RDONLY);
    if (fdr2 < 0)
    {
        std::cout << "Failed to open xillybus_read_16_2 device file" << endl;
        return -1;
    }

    // Start separate Writer & Reader processes.
    pid = fork();

    if (pid < 0)
    {
        std::cerr << "Failed to fork()" << endl;
        exit(1);
    }


    int64 t0 = cv::getTickCount();
    if (pid)
    {
        // Writer process (close the reader file descriptors)
        close(fdr1);
        close(fdr2);

        uint16_t* sdata_in      = new uint16_t[MAX_HEIGHT * MAX_WIDTH];
        uint16_t* sdata_size_in = new uint16_t[2];
        initDataPacket(sdata_in);

        cv::Mat src_yuv(rows, cols, CV_8UC2);
        cvtcolor_rgb2yuv422(src_rgb, src_yuv);

        sdata_size_in[0] = rows;
        sdata_size_in[1] = cols;
        memcpy(sdata_in, src_yuv.data, img_size * sizeof(uint16_t));

        // Write data on Xillybus in_data ports
        all_write(fdw1, sdata_in, img_size * sizeof(uint16_t));
        all_write(fdw2, sdata_size_in, 2 * sizeof(uint16_t));

        close(fdw1);
        close(fdw2);

        return 0;
    }
    else
    {
        // Reader process (close the writer file descriptors)
        close(fdw1);
        close(fdw2);

        uint16_t* sdata_out      = new uint16_t[MAX_HEIGHT * MAX_WIDTH];
        uint16_t* sdata_size_out = new uint16_t[2];
        initDataPacket(sdata_out);

        // Read data from Xillybus out_data ports
        all_read(fdr1, sdata_out, img_size * sizeof(uint16_t));
        all_read(fdr2, sdata_size_out, 2 * sizeof(uint16_t));

        int64 t1 = cv::getTickCount();
        double secs = (t1-t0)/cv::getTickFrequency();

        assert(rows == sdata_size_out[0]);
        assert(cols == sdata_size_out[1]);

        cv::Mat dst_yuv(rows, cols, CV_8UC2);
        cv::Mat dst_rgb(rows, cols, CV_8UC3);

        dst_yuv = cv::Mat(rows, cols, CV_8UC2, sdata_out).clone();
        cv::cvtColor(dst_yuv, dst_rgb, CV_YUV2BGR_YUY2); // CV_YUV2BGR_YUY2 = 115
        cv::imwrite(OUTPUT_IMAGE, dst_rgb);

        close(fdr1);
        close(fdr2);

        printf("Sobel Compute Time: %f\n", secs);

        return 0;
    }
}
