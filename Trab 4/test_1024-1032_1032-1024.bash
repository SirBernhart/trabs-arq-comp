#!/bin/bash
gcc -Wall -o matrix_generator matrix_generator.c
./matrix_generator 1024 1032 1024
gcc -Wall -mfma -o matrix_lib_test matrix_lib_test.c matrix_lib.c timer.c
./matrix_lib_test 1 1024 1032 1032 1024 matrix1.bin matrix2.bin result1.bin result2.bin