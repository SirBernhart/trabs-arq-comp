#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include "matrix_lib.h"

#define THREADS_PER_BLOCK_LIMIT 1024
#define BLOCKS_PER_GRID_LIMIT 65535

int threadsPerBlock = 256;
int maxBlocksPerGrid = 4096;

__global__ device_scalar_matrix_mult(int datasetSize, float scalar, float *matrixDeviceRows)
{
    int numThreads = gridDim.x * blockDim.x;
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int positionsToMultiply = (datasetSize + (numThreads - 1)) / numThreads;
    int initialPos = index * positionsToMultiply;

    float *auxMatrixPtr = matrixDeviceRows + initialPos;

    for (int i = 0; i < positionsToMultiply; i++, auxMatrixPtr++)
    {
        if (initialPos + i < datasetSize)
        {
            auxMatrixPtr *= scalar;
        }
    }
}

int scalar_matrix_mult(float scalar_value, struct matrix *matrix)
{
    int datasetSize = matrix->height * matrix->width;

    cudaError = cudaMemcpy(matrix->d_rows, matrix->h_rows, datasetSize * sizeof(float), cudaMemcpyHostToDevice);

    if (cudaError != cudaSuccess)
    {
        printf("cudaMemcpy (matrix->h_rows -> matrix->d_rows) returned error %s (code %d), line(%d)\n", cudaGetErrorString(cudaError), cudaError, __LINE__);
        return 0;
    }

    device_scalar_matrix_mult<<<maxBlocksPerGrid, threadsPerBlock>>>(datasetSize, matrix.d_rows);

    cudaDeviceSynchronize();

    cudaError = cudaMemcpy(matrix->h_rows, matrix->d_rows, datasetSize * sizeof(float), cudaMemcpyDeviceToHost);

    if (cudaError != cudaSuccess)
    {
        printf("cudaMemcpy (matrix->d_rows -> matrix->h_rows) returned error %s (code %d), line(%d)\n", cudaGetErrorString(cudaError), cudaError, __LINE__);
        return 0;
    }

    return 1;
}

__global__ device_matrix_matrix_mult(int datasetSize, float *matrixADeviceRows, float *matrixBDeviceRows, float *matrixCDeviceRows)
{
    int numThreads = gridDim.x * blockDim.x;
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int positionsToMultiply = (datasetSize + (numThreads - 1)) / numThreads;
    int initialPos = index * positionsToMultiply;

    float *auxMatrixAPtr = matrixADeviceRows + initialPos;
    float *auxMatrixBPtr = matrixBDeviceRows + initialPos;
    float *auxMatrixCPtr = matrixCDeviceRows + initialPos;

    int aColumn = 0;
    for (int aRow = 0; aRow < args->amountOfRowsToOperate; auxMatrixAPointer++)
    {
        auxMatrixCPointer = args->matCStartingRowPtr;
        auxMatrixCPointer += aRow * args->matCWidth;

        auxMatrixBPointer = args->matB->rows;
        auxMatrixBPointer += aColumn * args->matB->width;

        m256MatrixAPointer = _mm256_set1_ps(*auxMatrixAPointer);

        for (int column = 0; column < args->matB->width; auxMatrixBPointer += 8, auxMatrixCPointer += 8, column += 8)
        {
            m256MatrixBPointer = _mm256_load_ps(auxMatrixBPointer);
            m256MatrixCPointer = _mm256_load_ps(auxMatrixCPointer);
            m256MultAddResultPointer = _mm256_fmadd_ps(m256MatrixAPointer, m256MatrixBPointer, m256MatrixCPointer);

            _mm256_store_ps(auxMatrixCPointer, m256MultAddResultPointer);
        }

        if (aColumn + 1 == args->matAWidth)
        {
            aRow++;
            aColumn = 0;
        }
        else
        {
            aColumn++;
        }
    }
}

int matrix_matrix_mult(struct matrix *a, struct matrix *b, struct matrix *c)
{
    int datasetSizeA = a->height * a->width;

    int datasetSizeB = b->height * b->width;
    int datasetSizeC = c->height * c->width;

    cudaError = cudaMemcpy(a->d_rows, a->h_rows, datasetSizeA * sizeof(float), cudaMemcpyHostToDevice);

    if (cudaError != cudaSuccess)
    {
        printf("cudaMemcpy (a->h_rows -> a->d_rows) returned error %s (code %d), line(%d)\n", cudaGetErrorString(cudaError), cudaError, __LINE__);
        return 0;
    }
    cudaError = cudaMemcpy(b->d_rows, b->h_rows, datasetSizeB * sizeof(float), cudaMemcpyHostToDevice);

    if (cudaError != cudaSuccess)
    {
        printf("cudaMemcpy (b->h_rows -> b->d_rows) returned error %s (code %d), line(%d)\n", cudaGetErrorString(cudaError), cudaError, __LINE__);
        return 0;
    }

    cudaError = cudaMemcpy(c->d_rows, c->h_rows, datasetSizeC * sizeof(float), cudaMemcpyHostToDevice);

    if (cudaError != cudaSuccess)
    {
        printf("cudaMemcpy (c->h_rows -> c->d_rows) returned error %s (code %d), line(%d)\n", cudaGetErrorString(cudaError), cudaError, __LINE__);
        return 0;
    }

    device_matrix_matrix_mult<<<maxBlocksPerGrid, threadsPerBlock>>>(int datasetSizeC, a->d_rows, b->d_rows, c->d_rows);

    cudaDeviceSynchronize();

    cudaError = cudaMemcpy(c->h_rows, c->d_rows, datasetSizeC * sizeof(float), cudaMemcpyDeviceToHost);

    if (cudaError != cudaSuccess)
    {
        printf("cudaMemcpy (c->d_rows -> matrix->h_rows) returned error %s (code %d), line(%d)\n", cudaGetErrorString(cudaError), cudaError, __LINE__);
        return 0;
    }

    return 1;
}

int set_grid_size(int threads_per_block, int max_blocks_per_grid)
{
    if (threads_per_block > THREADS_PER_BLOCK_LIMIT || max_blocks_per_grid > BLOCKS_PER_GRID_LIMIT || threads_per_block <= 0 || max_blocks_per_grid <= 0)
    {
        printf("ERROR! Invalid CUDA amount for blocks (%d) or threads (%d)\n", threads_per_block, max_blocks_per_grid);
        return 0;
    }

    threadsPerBlock = threads_per_block;
    maxBlocksPerGrid = max_blocks_per_grid;

    return 1;
}