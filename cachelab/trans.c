/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 * 32 block, 8 int/per block
 * 
 * 32*32: need >= 32*8/32 = 8, 8*8 blocking, need 8 + 8 = 16 blocks in cache.
 *  A,B store adjoint, i*32 + j. j*32+i, if (i-j)%8 = 0, then hit convict
 *  and i-j<8, so if i=j, hit convict.
 * 64*64: need >= 32*8/64 = 4, 4*4 bloking, only need 8 blocks in cache, it is 
 *      too little. So, we can also use 8*8.But how to avoid confict? We can't 
 *      modify A, so we can modify B in the process. Divide 8*8 to up half and down
 *      half:
 *      ULA, URA | ULB, URB
 *      DLA, DRA | DLB, DRB
 *      first, use local var store ULB(a row),URB(a row), transpose and store in ULA(a col), URA(a col)
 *      second, move URA(a col) and DRB to the down half of A, then transpose DLB.
 * 61X67: not regular, it is too tired to kown the memory address and design a fit block.
 *      so, we can try 8*8, 16*16(better).
 *
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l, a1, a2, a3, a4, a5, a6, a7, a8;
    if(M == 32){ // 32X32
        for (i = 0; i < M; i += 8) {
            for (j = 0; j < N; j += 8) {
                for(k = i; k < i + 8; k ++){
                    a1 = A[k][j]; a2 = A[k][j+1]; a3 = A[k][j+2]; a4 = A[k][j+3];
                    a5 = A[k][j+4];a6 = A[k][j+5]; a7 = A[k][j+6]; a8 = A[k][j+7];
                    B[j][k] = a1; B[j+1][k] = a2; B[j+2][k] = a3; B[j+3][k] = a4;
                    B[j+4][k] = a5; B[j+5][k] = a6; B[j+6][k] = a7; B[j+7][k] = a8;
                }
            }
        }
    }else if(M == 64){ // 64X64
        for(i = 0; i < M; i += 8){
            for(j = 0; j < N; j += 8){
                // up half
                for(k = i; k < i+4; k++){
                    a1 = A[k][j]; a2 = A[k][j+1]; a3 = A[k][j+2]; a4 = A[k][j+3];
                    a5 = A[k][j+4];a6 = A[k][j+5]; a7 = A[k][j+6]; a8 = A[k][j+7];
                    B[j][k] = a1; B[j+1][k] = a2; B[j+2][k] = a3; B[j+3][k] = a4;
                    B[j][k+4] = a5; B[j+1][k+4] = a6; B[j+2][k+4] = a7; B[j+3][k+4] = a8;
                }
                // down half
                for(k = j + 4; k < j+8; k++){
                    //chunck 1: down right of B
                    a5 = A[i+4][k]; a6 = A[i+5][k]; a7 = A[i+6][k]; a8 = A[i+7][k];
                    //chunck 2: up right of A
                    a1 = B[k-4][i+4]; a2 = B[k-4][i+5]; a3 = B[k-4][i+6]; a4 = B[k-4][i+7];
                    //down left of A to be up right of  B
                    B[k-4][i+4] = A[i+4][k-4]; B[k-4][i+5] = A[i+5][k-4];
                    B[k-4][i+6] = A[i+6][k-4];B[k-4][i+7] = A[i+7][k-4];
                    //chunck 1 + chunck 2 to be down half of B 
                    B[k][i] = a1;B[k][i+1] = a2;B[k][i+2] = a3;B[k][i+3] = a4;
                    B[k][i+4] = a5;B[k][i+5] = a6;B[k][i+6] = a7;B[k][i+7] = a8;

                }

            }
        }

    }
    else if(M==61){  // 61X67 A NXM
         for(i = 0; i < N; i += 8){
            for(j = 0; j < M; j += 8){
                for(k = i;k<i+8 && k<N;k++){
                    for(l = j; l<j+8 && l < M;l++){
                        B[l][k] = A[k][l];
                    }
                }
            }
        }
    }


 
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

