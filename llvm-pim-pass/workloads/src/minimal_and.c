#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include "simdram.h"

// #define MEM_SIZE 2  // Keep it small to test behavior
// #define MEM_SIZE SIMDRAM_VECTOR_SIZE * 1 //65536
#define MEM_SIZE 2
// Ensure memory is aligned to 65536-byte boundary for full PIM optimization
// void *aligned_malloc(size_t alignment, size_t size) {
//     void *ptr = NULL;
//     if (posix_memalign(&ptr, alignment, size) != 0) {
//         perror("posix_memalign failed");
//         exit(EXIT_FAILURE);
//     }
//     return ptr;
// }

__attribute__((noinline))
int kernel(int *a, int *b, int *c, unsigned long size)
// int kernel_pim_and(uint8_t *a, uint8_t *b, uint8_t *c, unsigned long size)
{
    // clock_t begin = clock();
    for (int i = 0; i < size; i++)
        c[i] = a[i] & b[i];  //AND
    // clock_t end = clock();

    // double time = (double)(end - begin) / CLOCKS_PER_SEC;
    // printf("Time spent: %fs\n", time);
    return 0;
}

int main(void)
{
    int *a = malloc(sizeof(int[MEM_SIZE]));
    int *b = malloc(sizeof(int[MEM_SIZE]));
    int *c = malloc(sizeof(int[MEM_SIZE]));

    // simdram_mark_transposed(a);
    // simdram_mark_transposed(b);
    // simdram_mark_transposed(c);

    // Allocate aligned memory (65536-byte alignment for SIMD/PIM optimization)
    // int *a = (int*) aligned_malloc(SIMDRAM_VECTOR_SIZE * 4, MEM_SIZE * sizeof(int));
    // int *b = (int*) aligned_malloc(SIMDRAM_VECTOR_SIZE * 4, MEM_SIZE * sizeof(int));
    // int *c = (int*) aligned_malloc(SIMDRAM_VECTOR_SIZE * 4, MEM_SIZE * sizeof(int));

    // uint8_t *a = malloc(sizeof(uint8_t[MEM_SIZE]));
    // uint8_t *b = malloc(sizeof(uint8_t[MEM_SIZE]));
    // uint8_t *c = malloc(sizeof(uint8_t[MEM_SIZE]));
    
    for (int i = 0; i < MEM_SIZE; i++) {
        a[i] = 0b11101110;
        b[i] = 0b11100110;
    }
    // flush_cache();

    // for (int i = 0; i < MEM_SIZE; i++) {
    //     a[i] = 56;
    //     b[i] = 8;
    // }

    // kernel_pim_and(a, b, c, MEM_SIZE);
    kernel(a, b, c, MEM_SIZE);

    printf("Result: 0x%X\n", c[0]);
    // printf("Result: %d\n", c[0]);

    free(a);
    free(b);
    free(c);
    return 0;
}
