#include "matrix_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <immintrin.h>

int scalar_matrix_mult(float scalar_value, struct matrix *matrix)
{
    float *auxMatrixPointer = matrix->rows;

    int matrixArrayLength = matrix->height * matrix->width;
    float *lastMatrixAddress = auxMatrixPointer + matrixArrayLength;

    __m256 m256ScalarVector = _mm256_set1_ps(scalar_value);
    __m256 m256MatrixPointer;
    __m256 m256MultResultPointer;
    
    //float = 4 bytes (32bits) // 32 * 8 = 256 -> o tamanho do Vector __m256
    for (; auxMatrixPointer <= lastMatrixAddress; auxMatrixPointer += 8)
    {
        m256MatrixPointer = _mm256_load_ps(auxMatrixPointer);
        m256MultResultPointer = _mm256_mul_ps(m256ScalarVector, m256MatrixPointer);
        _mm256_store_ps(auxMatrixPointer, m256MultResultPointer);
    }

    return 1;
}

int matrix_matrix_mult(struct matrix *a, struct matrix *b, struct matrix *c)
{
    if (a->width != b->height)
    {
        printf("ERROR! Matrix a has a different number of columns (%lu) than Matrix b has rows (%lu)", a->width, b->height);
        return 0;
    }

    float *auxMatrixAPointer = a->rows;
    float *auxMatrixBPointer = b->rows;
    float *auxMatrixCPointer = c->rows;

    int matrixALength = a->height * a->width;

    float *lastMatrixAAddress = auxMatrixAPointer + matrixALength;

    int columnA = 0;

    __m256 m256MatrixAPointer;
    __m256 m256MatrixBPointer;
    __m256 m256MatrixCPointer;
    __m256 m256MultAddResultPointer;

    for (int row = 0; auxMatrixAPointer <= lastMatrixAAddress; auxMatrixAPointer++, columnA++)
    {
        if (columnA == a->width)
        {
            row++;
            columnA = 0;
        }

        auxMatrixCPointer = c->rows;
        auxMatrixCPointer += row * c->width;

        auxMatrixBPointer = b->rows;
        auxMatrixBPointer += row * b->width;

        m256MatrixAPointer = _mm256_set1_ps(*auxMatrixAPointer);

        for (int column = 0; column < b->width; auxMatrixBPointer+=8, auxMatrixCPointer+=8, column+=8)
        {
            m256MatrixBPointer = _mm256_load_ps(auxMatrixBPointer);
            m256MatrixCPointer = _mm256_load_ps(auxMatrixCPointer);
            m256MultAddResultPointer = _mm256_fmadd_ps(m256MatrixAPointer, m256MatrixBPointer, m256MatrixCPointer);

            _mm256_store_ps(auxMatrixCPointer, m256MultAddResultPointer);
        }
    }

    return 1;
}