# sobel_edge_detection_zedboard
Implementing Sobel edge detection algorithm on a ZedBoard

Source Tree:
============

sobel_edge_detection_zedboard
├── README.txt
├── report
│   ├── hand_written
│   │   └── Sobel_Edge_Detection_on_Xillinx_ZedBoard_FPGA_HW.pdf
│   └── soft_copy
│       └── Sobel_Edge_Detection_on_Xillinx_ZedBoard_FPGA.pdf
├── src
│   ├── opencv_standalone
│   │   └── opencv_sed_test.cpp
│   ├── vivado_hls_zedboard
│   │   ├── zboard_sed_test.cpp
│   │   ├── zboard_sed_top.cpp
│   │   └── zboard_sed_top.h
│   └── xillinux_test_sed
│       ├── Makefile
│       └── test_sobel.cpp
├── test_images
│   ├── input_adiyogi.jpg
│   ├── output_adiyogi_sed_x86.jpg
│   └── output_adiyogi_sed_zedboard.jpg
├── vivado_block_design
│   ├── sed_ip_core_block_design.pdf
│   └── zynq_full_system_block_design.pdf
└── xillybus_custom_core
    └── xillybus_ip_core_for_sed.pdf

