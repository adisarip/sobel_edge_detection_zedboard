CFLAGS=-g -Wall -O3 -std=c++11
LDFLAGS=`pkg-config --cflags --libs opencv`
SOURCE_FILES=test_sobel.cpp

all: 	test_sobel

test_sobel:	test_sobel.cpp
	g++ $(SOURCE_FILES) $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f test_sobel
