#include <stdlib.h>
#include <stdio.h>
#include <time.h>
// #include "simdram.h"

// 4GB of data in each array
// #define MEM_SIZE SIMDRAM_VECTOR_SIZE * 10000
// #define MEM_SIZE SIMDRAM_VECTOR_SIZE * 10000
// #define MEM_SIZE 65536 * 10000
#define MEM_SIZE 10 
#define COLS_A 5
#define ROWS_B COLS_A // 
#define COLS_B 8
#define COLS_C COLS_B
// #define ROWS MEM_SIZE   // Number of rows
// #define COLS 20   // Number of columns

void printVector(int *v1, unsigned long size, char *str){
    printf("\nPrinting Vector: %s\n", str);
    for (size_t i = 0; i < size; i++)
    {
        printf("%d ", v1[i]);
    }
    printf("\n");
}
void printMatrix(int **mat, unsigned long row_size, unsigned long col_size, char *str){
    printf("\nPrinting Matrix %s\n", str);
    for (size_t i = 0; i < row_size; i++)
    {
        printf("\nRow%d\t",i);
        for (size_t j = 0; j < col_size; j++)
        {
            printf("%d ", mat[i][j]);
        }
        
    }
    printf("\n");
    
}

__attribute__((noinline))
// int** convertVectorToMatrix(int *a, unsigned long COLS_A, unsigned long COLS_B){
int** convertVectorToMatrix(int *a){
    int **d = malloc(ROWS_B * sizeof(int *));
    if (d == NULL) {
        perror("Failed to allocate memory for rows");
        return -1;
    }

    int i=0, j=0;
    for(int i=0; i< COLS_A ; i++){
        
        d[i] = malloc(COLS_B * sizeof(int));
        if(d[i] == NULL){
            printf("Failed to allocate mem for %d row\n", i);
            for(j=0; j<i; j++){
                free(d[j]);
            }
        }
    }

    //copy the data from a to d
    for(i=0; i< COLS_A; i++){
        for(j=0; j< COLS_B; j++){
            d[i][j] = a[i];
        }
    }
    return d;
}

__attribute__((noinline))
// int kernel(int *a, int *b, int *c, unsigned long size)
int kernel(int **d, int **b, int *c)
{
    // clock_t begin = clock();
    // for (int i = 0; i < size; i++)
    //     c[i] = a[i] & b[i];
    // // clock_t end = clock();

    // double time = (double)(end - begin) / CLOCKS_PER_SEC;
    // printf("Time spent: %fs\n", time);
    //elementwise multiplication of d and b for each row 
        //sum for each column
    int i=0, j=0;
    for ( i = 0; i < ROWS_B; i++)
    {
        for ( j = 0; j < COLS_B; j++)
        {
            //
            c[j] = c[j] + d[i][j] * b[i][j];
        }
        // c[i] = c[i] + ans[i];
    }
    return 0;
}

void init(int *a, int **b){

    for (int i = 0; i < COLS_A; i++)
    {
        a[i] = i;
    }

    //init b
    for (int i = 0; i < ROWS_B; i++)
    {
        for (int j = 0; j < COLS_B; j++)
        {
            b[i][j] = 1;
        }
        
    }
    
    
}

int* vectorMatrixMul(int *a, int **b, unsigned long cols_a, unsigned long cols_b){
    int rows_b = cols_a;
    int *ans = (int *)malloc(cols_b * sizeof(int));

    for (int j = 0; j < cols_b; j++)
    {
        for (int i = 0; i < rows_b; i++) //col_a
        {
            ans[j] = ans[j] + b[i][j] * a[i];
        }
        
    }
    return ans;
    
}
int main(void)
{
    int *a = malloc(sizeof(int[COLS_A]));
    // int *b = malloc(sizeof(int[MEM_SIZE]));
    int *c = malloc(sizeof(int[COLS_C]));
    int **b = malloc(ROWS_B * sizeof(int *));
    if (b == NULL) {
        perror("Failed to allocate memory for rows");
        return -1;
    }

    //allocate  mem for each rows
        //allocate mem for each col
    int i=0, j=0;
    for(int i=0; i< ROWS_B ; i++){
        
        b[i] = malloc(COLS_B * sizeof(int));
        if(b[i] == NULL){
            printf("Failed to allocate mem for %d row\n", i);
            for(j=0; j<i; j++){
                free(b[j]);
            }
        }
        // for(int j=0; j < COLS; j++){
        //     b[i]
        // }
    }

    //init data for a & b
    init(a,b);
    //d is same size as b --> elementwise multiplication operation
    int **d = convertVectorToMatrix(a);
    printVector(a, COLS_A, "a");
    printMatrix(d, ROWS_B, COLS_B, "d");
    printMatrix(b, ROWS_B, COLS_B, "b");

    int *ans = vectorMatrixMul(a, b, COLS_A, COLS_B);
    printVector(ans, COLS_C, "ans");

    kernel(d, b, c);

    //print output
    printf("Printing out\n");
    for (i = 0; i < COLS_C; i++)
    {
       printf("%d ", c[i]);
    }
    printf("\n");

   
    
    free(a);
    free(b);
    free(c);
    free(d);

    return 0;
}