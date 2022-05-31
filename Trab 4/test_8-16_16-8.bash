#!/bin/bash
gcc -Wall -o matrix_generator matrix_generator.c
./matrix_generator 8 16 8
nvcc -o matrix_lib_test matrix_lib_test.cu matrix_lib.cu timer.c
./matrix_lib_test 2 8 16 16 8 256 4096 matrix1.bin matrix2.bin result1.bin result2.bin