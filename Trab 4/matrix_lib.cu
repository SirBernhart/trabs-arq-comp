#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include "matrix_lib.h"

#define THREADS_PER_BLOCK_LIMIT 1024
#define BLOCKS_PER_GRID_LIMIT 65535

int threadsPerBlock = 256;
int maxBlocksPerGrid = 4096;

__global__ void device_scalar_matrix_mult(int datasetSize, float scalar, float *matrixDeviceRows)
{
    int numThreads = gridDim.x * blockDim.x;
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int positionsToMultiply = (datasetSize + (numThreads - 1)) / numThreads;
    int initialPos = index * positionsToMultiply;
    int currIndex = initialPos;
    
    float *auxMatrixPtr = matrixDeviceRows + initialPos;

    if(initialPos >= datasetSize)
    {
        return;
    }

    for (int i = 0 ; i < positionsToMultiply ; i++, auxMatrixPtr++)
    {
        if (initialPos + i < datasetSize)
        {
            *auxMatrixPtr = scalar * *auxMatrixPtr;
        }
    }
}

int scalar_matrix_mult(float scalar_value, struct matrix *matrix)
{
    int datasetSize = matrix->height * matrix->width;

    cudaError_t cudaError = cudaMemcpy(matrix->d_rows, matrix->h_rows, datasetSize * sizeof(float), cudaMemcpyHostToDevice);

    if (cudaError != cudaSuccess)
    {
        printf("cudaMemcpy (matrix->h_rows -> matrix->d_rows) returned error %s (code %d), line(%d)\n", cudaGetErrorString(cudaError), cudaError, __LINE__);
        return 0;
    }

    device_scalar_matrix_mult<<<maxBlocksPerGrid, threadsPerBlock>>>(datasetSize, scalar_value, matrix->d_rows);

    cudaDeviceSynchronize();

    cudaError = cudaMemcpy(matrix->h_rows, matrix->d_rows, datasetSize * sizeof(float), cudaMemcpyDeviceToHost);

    if (cudaError != cudaSuccess)
    {
        printf("cudaMemcpy (matrix->d_rows -> matrix->h_rows) returned error %s (code %d), line(%d)\n", cudaGetErrorString(cudaError), cudaError, __LINE__);
        return 0;
    }

    return 1;
}

__global__ void device_matrix_matrix_mult(int datasetSize, int aWidth, float *aRows, int bWidth, float *bRows, int cWidth, float *cRows)
{
    int numThreads = gridDim.x * blockDim.x;
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int rowsToMultiply = (datasetSize + (numThreads - 1)) / numThreads;
    int initialPos = index * rowsToMultiply;

    float *auxMatrixAPtr = aRows + initialPos * aWidth;
    float *auxMatrixBPtr = bRows;
    float *auxMatrixCPtr = cRows + initialPos * cWidth;

    int aColumn = 0;

    if(initialPos >= datasetSize)
    {
        return;
    }

    for (int aCurrRow = 0 ; aCurrRow < rowsToMultiply ; auxMatrixAPtr++)
    {
        if(initialPos + aCurrRow >= datasetSize)
        {
            break;
        }

        auxMatrixCPtr = cRows + initialPos * cWidth;
        auxMatrixCPtr += aCurrRow * cWidth;

        auxMatrixBPtr = bRows;
        auxMatrixBPtr += aColumn * bWidth;

        for (int column = 0; column < bWidth; auxMatrixBPtr++, auxMatrixCPtr++, column++)
        {
            *auxMatrixCPtr += *auxMatrixAPtr * (*auxMatrixBPtr);
        }

        if (aColumn + 1 == aWidth)
        {
            aCurrRow++;
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
    int datasetSizeA = a->height * a->width * sizeof(float);
    int datasetSizeB = b->height * b->width * sizeof(float);
    int datasetSizeC = c->height * c->width * sizeof(float);

    cudaError_t cudaError = cudaMemcpy(a->d_rows, a->h_rows, datasetSizeA, cudaMemcpyHostToDevice);

    if (cudaError != cudaSuccess)
    {
        printf("cudaMemcpy (a->h_rows -> a->d_rows) returned error %s (code %d), line(%d)\n", cudaGetErrorString(cudaError), cudaError, __LINE__);
        return 0;
    }

    cudaError = cudaMemcpy(b->d_rows, b->h_rows, datasetSizeB, cudaMemcpyHostToDevice);

    if (cudaError != cudaSuccess)
    {
        printf("cudaMemcpy (b->h_rows -> b->d_rows) returned error %s (code %d), line(%d)\n", cudaGetErrorString(cudaError), cudaError, __LINE__);
        return 0;
    }

    cudaError = cudaMemcpy(c->d_rows, c->h_rows, datasetSizeC, cudaMemcpyHostToDevice);

    if (cudaError != cudaSuccess)
    {
        printf("cudaMemcpy (c->h_rows -> c->d_rows) returned error %s (code %d), line(%d)\n", cudaGetErrorString(cudaError), cudaError, __LINE__);
        return 0;
    }

    device_matrix_matrix_mult<<<maxBlocksPerGrid, threadsPerBlock>>>(c->height, a->width, a->d_rows, b->width, b->d_rows, c->width, c->d_rows);

    cudaDeviceSynchronize();

    cudaError = cudaMemcpy(c->h_rows, c->d_rows, datasetSizeC, cudaMemcpyDeviceToHost);
    
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