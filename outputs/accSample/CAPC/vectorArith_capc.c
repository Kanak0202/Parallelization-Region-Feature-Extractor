// C program to implement Vector Arithmetic

#include <stdio.h>

#define SIZE 13000000

int main()
{
    double A[13000000];
    double B[13000000];
    double C[13000000];
    double D[13000000];
    double E[13000000];

    int i = 0;

    // Array initialization

#pragma capc profitability_region begin
#pragma acc parallel loop auto gang vector num_gangs(50782) vector_length(256)
    for (i = 0; i <= 12999999; i += 1) {
        A[i] = ((double)i);
        B[i] = ((double)(i + 1));
    }
#pragma capc profitability_region end


    // C = A + B

#pragma capc profitability_region begin
#pragma acc parallel loop auto gang vector num_gangs(50782) vector_length(256)
    for (i = 0; i <= 12999999; i += 1) {
        C[i] = A[i] + B[i];
    }
#pragma capc profitability_region end


    // Verify result

    for (i = 0; i <= 12999999; i += 1) {
        if (C[i] != A[i] + B[i]) {
            printf("Add : Something didn't work correctly!\n");
            break;
        }
    }

    if (i == 13000000) {
        printf("Add : Everything seems to work fine! \n");
    }


    // D = A - B

#pragma capc profitability_region begin
#pragma acc parallel loop auto gang vector num_gangs(50782) vector_length(256)
    for (i = 0; i <= 12999999; i += 1) {
        D[i] = A[i] - B[i];
    }
#pragma capc profitability_region end


    // Verify result

    for (i = 0; i <= 12999999; i += 1) {
        if (D[i] != A[i] - B[i]) {
            printf("Sub : Something didn't work correctly!\n");
            break;
        }
    }

    if (i == 13000000) {
        printf("Sub : Everything seems to work fine! \n");
    }


    // E = A * B

#pragma capc profitability_region begin
#pragma acc parallel loop auto gang vector num_gangs(50782) vector_length(256)
    for (i = 0; i <= 12999999; i += 1) {
        E[i] = A[i] * B[i];
    }
#pragma capc profitability_region end


    // Verify result

    for (i = 0; i <= 12999999; i += 1) {
        if (E[i] != A[i] * B[i]) {
            printf("Mult : Something didn't work correctly!\n");
            break;
        }
    }

    if (i == 13000000) {
        printf("Mult : Everything seems to work fine! \n");
    }

    return 0;
}