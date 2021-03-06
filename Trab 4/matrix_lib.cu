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
    
    float *auxMatrixPtr = matrixDeviceRows + initialPos;

    for (int i = 0 ; i < positionsToMultiply ; i++, auxMatrixPtr++)
    {
        if (initialPos + i < datasetSize)
        {
            *auxMatrixPtr *= scalar;
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
    int elementsToCalculate = (datasetSize + (numThreads - 1)) / numThreads;
    int initialPos = index * elementsToCalculate;

    float *auxMatrixAPtr;
    float *auxMatrixBPtr;
    float *auxMatrixCPtr;

    if(initialPos >= datasetSize)
    {
        return;
    }
    auxMatrixCPtr = cRows + initialPos;

    for(int elementsCalculated = 0 ; elementsCalculated < elementsToCalculate ; elementsCalculated++)
    {
        if(initialPos + elementsCalculated >= datasetSize)
        {
            return;
        }
        auxMatrixAPtr = aRows + (initialPos/cWidth) * aWidth;
        auxMatrixBPtr = bRows + (initialPos % bWidth);

        for(int aColumnFromInitPos = 0 ; aColumnFromInitPos < aWidth ; aColumnFromInitPos++, auxMatrixAPtr++, auxMatrixBPtr += bWidth)
        {
            *auxMatrixCPtr += *auxMatrixAPtr * (*auxMatrixBPtr);
        }
        auxMatrixCPtr++;
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

    device_matrix_matrix_mult<<<maxBlocksPerGrid, threadsPerBlock>>>(datasetSizeC/sizeof(float), a->width, a->d_rows, b->width, b->d_rows, c->width, c->d_rows);

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