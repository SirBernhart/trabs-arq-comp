#include <stdio.h>
#include <stdlib.h>

float* generate_matrix(float rows, float cols)
{
    float* matrix = (float*) malloc(rows * cols * sizeof(float));
    float* auxMatrixPtr = matrix;

    for(float row = 0 ; row < rows ; row++)
    {
        for(float col = 0 ; col < cols ; col++, auxMatrixPtr++)
        {
            *auxMatrixPtr = col + 1;
        }
    }

    return matrix;
}

int main(int argc, char *argv[])
{
    FILE *fp;

    float rowsA = strtof(argv[1], NULL);
    float colsARowsB = strtof(argv[2], NULL);
    float colsB = strtof(argv[3], NULL);

    float* matrix1 = generate_matrix(rowsA, colsARowsB); 
    float* matrix2 = generate_matrix(colsARowsB, colsB); 

    fp = fopen("matrix1.bin", "wb");
    if (fp == NULL)
    {
        printf("Error: file matrix1.bin cannot be opened\n");
        exit(1);
    }

    fwrite(matrix1, sizeof(float), 128, fp);

    fclose(fp);

    fp = fopen("matrix2.bin", "wb");
    if (fp == NULL)
    {
        printf("Error: file matrix2.bin cannot be opened\n");
        exit(1);
    }

    fwrite(matrix2, sizeof(float), 128, fp);

    fclose(fp);

    free(matrix1);
    free(matrix2);
}
