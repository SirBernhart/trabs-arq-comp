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
    float* matrixStartingRowPtr;
    float scalarValue;
    int matrixArraySize;
} SCALAR_MATRIX_MULT_THREAD_ARGS;

typedef struct
{
    int matAWidth;
    float* matAStartingRowPtr;
    int matCWidth;
    float* matCStartingRowPtr;
    struct matrix* matB;
    int amountOfRowsToOperate;
} MATRIX_MATRIX_MULT_THREAD_ARGS;

// =========== AUX FUNCTIONS =========== 
int check_scalar_matrix_mult_correctness(struct matrix *matrix);
int check_matrix_matrix_mult_correctness(struct matrix *a, struct matrix *b, struct matrix *c);
//COMMON_THREAD_ARGS create_common_thread_args(int matrixArraySize, unsigned long matrixWidth, float *startingRowPtr);
int get_amount_of_rows_to_operate(unsigned long matrixHeight);
void create_threads(void* args, void* threadFunction, int sizeofArgsStruct);

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

    SCALAR_MATRIX_MULT_THREAD_ARGS args[threadAmount];

    float* auxMatrixRowsPointer = matrix->rows;
    int amountOfRowsToOperate = get_amount_of_rows_to_operate(matrix->height);
    int matrixArraySize = amountOfRowsToOperate * matrix->width;
    for(int i = 0 ; i < threadAmount ; i++, auxMatrixRowsPointer += matrixArraySize)
    {
        args[i].matrixArraySize = matrixArraySize;
        args[i].matrixWidth = matrix->width;
        args[i].matrixStartingRowPtr = auxMatrixRowsPointer;
        args[i].scalarValue = scalar_value;
    }

    create_threads(args, thread_scalar_matrix_mult, sizeof(SCALAR_MATRIX_MULT_THREAD_ARGS));
    
    return 1;
}

void *thread_scalar_matrix_mult(void* pthreadArgs)
{
    SCALAR_MATRIX_MULT_THREAD_ARGS *args = (SCALAR_MATRIX_MULT_THREAD_ARGS*)pthreadArgs;

    float *auxMatrixPointer = args->matrixStartingRowPtr;

    float *lastMatrixAddress = auxMatrixPointer + args->matrixArraySize;

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
    float *auxMatrixCPointer = c->rows;

    MATRIX_MATRIX_MULT_THREAD_ARGS args[threadAmount];

    int amountOfRowsToOperate = get_amount_of_rows_to_operate(c->height);
    int matrixAArraySize = amountOfRowsToOperate * a->width;
    int matrixCArraySize = amountOfRowsToOperate * c->width;
    for(int i = 0 ; i < threadAmount ; i++, auxMatrixAPointer+=matrixAArraySize, auxMatrixCPointer+=matrixCArraySize)
    {
        args[i].matAWidth = a->width;
        args[i].matAStartingRowPtr = auxMatrixAPointer;
        args[i].matCWidth = c->width;
        args[i].matCStartingRowPtr = auxMatrixCPointer;
        args[i].matB = b;
        args[i].amountOfRowsToOperate = amountOfRowsToOperate;
    }

    create_threads(args, thread_matrix_matrix_mult, sizeof(MATRIX_MATRIX_MULT_THREAD_ARGS));

    return 1;
}

void *thread_matrix_matrix_mult(void* pthreadArgs)
{
    MATRIX_MATRIX_MULT_THREAD_ARGS* args = (MATRIX_MATRIX_MULT_THREAD_ARGS*) pthreadArgs;

    float *auxMatrixAPointer = args->matAStartingRowPtr;
    float *auxMatrixBPointer = args->matB->rows;
    float *auxMatrixCPointer = args->matCStartingRowPtr;

    __m256 m256MatrixAPointer;
    __m256 m256MatrixBPointer;
    __m256 m256MatrixCPointer;
    __m256 m256MultAddResultPointer;

    int columnA = 0;
    for (int rowA = 0 ; rowA < args->amountOfRowsToOperate ; auxMatrixAPointer++)
    {
        auxMatrixCPointer = args->matCStartingRowPtr;
        auxMatrixCPointer += rowA * args->matCWidth;

        auxMatrixBPointer = args->matB->rows;
        auxMatrixBPointer += columnA * args->matB->width;

        m256MatrixAPointer = _mm256_set1_ps(*auxMatrixAPointer);

        for (int column = 0; column < args->matB->width; auxMatrixBPointer+=8, auxMatrixCPointer+=8, column+=8)
        {
            m256MatrixBPointer = _mm256_load_ps(auxMatrixBPointer);
            m256MatrixCPointer = _mm256_load_ps(auxMatrixCPointer);
            m256MultAddResultPointer = _mm256_fmadd_ps(m256MatrixAPointer, m256MatrixBPointer, m256MatrixCPointer);

            _mm256_store_ps(auxMatrixCPointer, m256MultAddResultPointer);
        }

        if (columnA+1 == args->matAWidth)
        {
            rowA++;
            columnA = 0;
        }
        else
        {
            columnA++;
        }
    }

    pthread_exit(NULL);
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

void create_threads(void* args, void* threadFunction, int sizeofArgsStruct)
{
    pthread_t idThreads[threadAmount];
    pthread_attr_t threadAttribute;
    int threadCreateRetVal, threadJoinRetVal;
    void* pStatus;

    pthread_attr_init(&threadAttribute);
    pthread_attr_setdetachstate(&threadAttribute, PTHREAD_CREATE_JOINABLE);

    for(int i = 0 ; i < threadAmount ; i++, args += sizeofArgsStruct)
    {
        threadCreateRetVal = pthread_create(&idThreads[i], &threadAttribute, threadFunction, args);

        if(threadCreateRetVal != 0)
        {
            fprintf(stderr, "ERROR! Thread creation failed with error %d\n", threadCreateRetVal);
            exit(-1);
        }
    }

    for(int i = 0 ; i < threadAmount ; i++)
    {
        threadJoinRetVal = pthread_join(idThreads[i], &pStatus);
        if(threadJoinRetVal != 0)
        {
            fprintf(stderr, "ERROR! Thread join creation failed with error %d\n", threadJoinRetVal);
            exit(-1);
        }
    }
}

unsigned long get_number_threads()
{
    return (unsigned long) threadAmount;
}

int check_thread_matrix_correctness(struct matrix* matrix, char* matrixName)
{
    if(matrix->height % threadAmount != 0 || threadAmount > matrix->height)
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