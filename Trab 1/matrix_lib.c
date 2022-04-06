#include "matrix_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>

// ./matrix_lib_test 5 3 3 3 3 matrix1.bin matrix2.bin result1.bin result2.bin
// gcc -Wall -o matrix_lib_test matrix_lib_test.c matrix_lib.c timer.c

int scalar_matrix_mult(float scalar_value, struct matrix *matrix)
{
    float *auxMatrixPointer = matrix->rows;

    int matrixArrayLength = matrix->height * matrix->width;
    float *lastMatrixAddress = auxMatrixPointer + (matrixArrayLength * sizeof(float *));

    for (; auxMatrixPointer <= lastMatrixAddress; auxMatrixPointer++)
    {
        *auxMatrixPointer *= scalar_value;
    }

    return 1;
}

int matrix_matrix_mult(struct matrix *a, struct matrix *b, struct matrix *c)
{
    if (a->width != b->height)
    {
        printf("ERROR! Matrix a has a different number of columns (%d) than Matrix b has rows (%d)", &a->width, &b->height);
        return 0;
    }

    float *auxMatrixAPointer = a->rows;
    float *auxMatrixBPointer = b->rows;
    float *auxMatrixCPointer = c->rows;

    int matrixALength = a->height * a->width;
    // int matrixBLength = b->height * b->width;
    // int matrixCLength = c->height * c->width;

    float *lastMatrixAAddress = auxMatrixAPointer + (matrixALength * sizeof(float *));
    // float *lastMatrixBAddress = auxMatrixBPointer + (matrixBLength * sizeof(float *));
    // float *lastMatrixCAddress = auxMatrixCPointer + (matrixCLength * sizeof(float *));

    for (int row = 0; auxMatrixAPointer < lastMatrixAAddress; auxMatrixAPointer++, row++)
    {
        for (int column = 0; column < b->width; auxMatrixCPointer = c->rows + row * b->width, auxMatrixBPointer++, column++, auxMatrixCPointer++)
        {

            *auxMatrixCPointer += (*auxMatrixAPointer) * (*auxMatrixBPointer);
        }
    }

    return 1;
}