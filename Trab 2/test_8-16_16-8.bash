#!/bin/bash
gcc -Wall -o matrix_generator matrix_generator.c
./matrix_generator 8 16 8
gcc -Wall -mfma -o matrix_lib_test matrix_lib_test.c matrix_lib.c timer.c
./matrix_lib_test 1 8 16 16 8 matrix1.bin matrix2.bin result1.bin result2.bin