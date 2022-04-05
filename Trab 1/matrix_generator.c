#include <stdio.h>
#include <stdlib.h>

int main() 
{
    FILE *fp;
    
    float matrix1[3][3] = 
    {{1.0, 2.0, 3.0},
    {1.0, 2.0, 3.0},
    {1.0, 2.0, 3.0}};

    float matrix2[3][3] = 
    {{1.0, 2.0, 3.0},
    {1.0, 2.0, 3.0},
    {1.0, 2.0, 3.0}};

    fp = fopen("matrix1.bin", "w");
    if (fp == NULL)
    {
        printf("Error: file matrix1.bin cannot be opened\n");
        exit(1);
    }

    fwrite(matrix1, sizeof(float*), 9, fp);

    fclose(fp);

    fp = fopen("matrix2.bin", "w");
    if (fp == NULL)
    {
        printf("Error: file matrix2.bin cannot be opened\n");
        exit(1);
    }

    fwrite(matrix2, sizeof(float*), 9, fp);

    fclose(fp);
}