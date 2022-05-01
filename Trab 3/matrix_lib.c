#include "matrix_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <immintrin.h>
#include <pthread.h>

int threadAmount = 1;

typedef struct 
{
    int matrixWidth;
    int amountOfRowsToOperate;
    float* matrixStartingRowPtr;
} COMMON_THREAD_ARGS;

typedef struct
{
    COMMON_THREAD_ARGS commonArgs;
    float scalarValue;
} SCALAR_MATRIX_MULT_THREAD_ARGS;

typedef struct
{
    COMMON_THREAD_ARGS commonArgs;
    struct matrix *matA;
    struct matrix *matB;
} MATRIX_MATRIX_MULT_THREAD_ARGS;

// =========== AUX FUNCTIONS =========== 
int check_scalar_matrix_mult_correctness(struct matrix *matrix);
int check_matrix_matrix_mult_correctness(struct matrix *a, struct matrix *b, struct matrix *c);
COMMON_THREAD_ARGS create_common_thread_args(int amountOfRowsToOperate, unsigned long matrixWidth, float *startingRowPtr);
int get_amount_of_rows_to_operate(unsigned long matrixHeight);
int create_threads();

// =========== THREAD FUNCTIONS =========== 
void *thread_scalar_matrix_mult(void* args);
void *thread_matrix_matrix_mult();

// =========== MAIN FUNCTIONS ===========

int scalar_matrix_mult(float scalar_value, struct matrix *matrix)
{
    if(!check_scalar_matrix_mult_correctness(matrix))
    {
        return 0;
    }

    pthread_t idThreads[threadAmount];
    pthread_attr_t threadAttribute;
    SCALAR_MATRIX_MULT_THREAD_ARGS args[threadAmount];
    int threadCreateRetVal;
    int threadJoinRetVal;
    void* pStatus;

    pthread_attr_init(&threadAttribute);
    pthread_attr_setdetachstate(&threadAttribute, PTHREAD_CREATE_JOINABLE);

    float* auxMatrixRowsPointer = matrix->rows;
    int amountOfRowsToOperate = get_amount_of_rows_to_operate(matrix->height);
    for(int i = 0 ; i < threadAmount ; i++, auxMatrixRowsPointer += amountOfRowsToOperate * matrix->width)
    {
        args[i].commonArgs = create_common_thread_args(amountOfRowsToOperate, matrix->width, auxMatrixRowsPointer);
        args[i].scalarValue = scalar_value;
    }

    for(int i = 0 ; i < threadAmount ; i++)
    {
        threadCreateRetVal = pthread_create(&idThreads[i], &threadAttribute, thread_scalar_matrix_mult, &args[i]);

        if(threadCreateRetVal != 0)
        {
            fprintf(stderr, "ERROR! Thread creation failed with error %d\n", threadCreateRetVal);
            exit(-1);
        }

        threadJoinRetVal = pthread_join(idThreads[i], &pStatus);
        if(threadJoinRetVal != 0)
        {
            fprintf(stderr, "ERROR! Thread join creation failed with error %d\n", threadJoinRetVal);
            exit(-1);
        }
    }
    
    return 1;
}

void *thread_scalar_matrix_mult(void* pthreadArgs)
{
    SCALAR_MATRIX_MULT_THREAD_ARGS *args = (SCALAR_MATRIX_MULT_THREAD_ARGS*)pthreadArgs;

    float *auxMatrixPointer = args->commonArgs.matrixStartingRowPtr;
    int matrixArrayLength = args->commonArgs.amountOfRowsToOperate * args->commonArgs.matrixWidth;

    float *lastMatrixAddress = auxMatrixPointer + matrixArrayLength;

    __m256 m256ScalarVector = _mm256_set1_ps(args->scalarValue);
    __m256 m256MatrixPointer;
    __m256 m256MultResultPointer;

    //float = 4 bytes (32bits) // 32 * 8 = 256 -> o tamanho do Vector __m256
    for (; auxMatrixPointer < lastMatrixAddress; auxMatrixPointer += 8)
    {
        m256MatrixPointer = _mm256_load_ps(auxMatrixPointer);
        m256MultResultPointer = _mm256_mul_ps(m256ScalarVector, m256MatrixPointer);
        _mm256_store_ps(auxMatrixPointer, m256MultResultPointer);
    }

    pthread_exit(NULL);
}

int matrix_matrix_mult(struct matrix *a, struct matrix *b, struct matrix *c)
{
    if(!check_matrix_matrix_mult_correctness(a, b, c))
    {
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

    for (int row = 0; auxMatrixAPointer < lastMatrixAAddress; auxMatrixAPointer++, columnA++)
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

void set_number_threads(int num_threads)
{
    if(num_threads <= 0)
    {
        fprintf(stderr, "ERROR! Number of threads should be higher than 0! Amount set: %d", num_threads);
        return;
    }

    threadAmount = num_threads;
}

unsigned long get_number_threads()
{
    return (unsigned long) threadAmount;
}

int check_thread_matrix_correctness(struct matrix* matrix, char* matrixName)
{
    if(matrix->height % threadAmount != 0)
    {
        fprintf(stderr, "ERROR! Matrix %s's number of rows (%lu) must be a multiple of the number of threads (amount set: %d)\n", matrixName, matrix->height, threadAmount);
        return 0;
    }

    if(matrix->width % 8 != 0)
    {
        fprintf(stderr, "ERROR! Matrix %s's number of columns (%lu) must be a multiple of 8\n", matrixName, matrix->width);
        return 0;
    }

    return 1;
}

int check_scalar_matrix_mult_correctness(struct matrix *matrix)
{
    if(matrix == NULL)
    {
        fprintf(stderr, "ERROR! The matrix passed to the scalar multiplication is NULL!");
        return 0;
    }

    if(!check_thread_matrix_correctness(matrix, "Scalar mult"))
    {
        return 0;
    }

    return 1;
}

int check_matrix_matrix_mult_correctness(struct matrix *a, struct matrix *b, struct matrix *c)
{
    if(a == NULL || b == NULL || c == NULL)
    {
        fprintf(stderr, "ERROR! One or more matrices passed to the matrix on matrix multiplication are NULL!");
        return 0;
    }

    if (a->width != b->height)
    {
        fprintf(stderr, "ERROR! Matrix A has a different number of columns (%lu) than Matrix B has rows (%lu)\n", a->width, b->height);
        return 0;
    }

    if(!check_thread_matrix_correctness(c, "C"))
    {
        return 0;
    }

    return 1;
}

int get_amount_of_rows_to_operate(unsigned long matrixHeight)
{
    return matrixHeight/threadAmount;
}

COMMON_THREAD_ARGS create_common_thread_args(int amountOfRowsToOperate, unsigned long matrixWidth, float *startingRowPtr)
{
    COMMON_THREAD_ARGS args;

    args.amountOfRowsToOperate = amountOfRowsToOperate;
    args.matrixStartingRowPtr = startingRowPtr;
    args.matrixWidth = matrixWidth;

    return args;
}