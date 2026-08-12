#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#define n 10

int main()
{
	int i, j, k;

	float A[n][n][n],B[n][n][n];

    #pragma acc enter data create(A[0:n][0:n][0:n],B[0:n][0:n][0:n])

    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(3) present(A,B)
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			for (k = 0; k < n; k++)
				A[i][j][k] = B[i][j][k] = (float) (i + j + (n-k))* 10 / (n);
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(3) present(A,B)
	for (i = 1; i < n-1; i++) {
		for (j = 1; j < n-1; j++) {
			for (k = 1; k < n-1; k++) {
				B[i][j][k] = 0.125 * (A[i+1][j][k] - (2.0) * A[i][j][k] + A[i-1][j][k])
					+ 0.125 * (A[i][j+1][k] - (2.0) * A[i][j][k] + A[i][j-1][k])
					+ 0.125 * (A[i][j][k+1] -(2.0) * A[i][j][k] + A[i][j][k-1])
					+ A[i][j][k];
			}
		}
	}
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(3) present(A,B)
	for (i = 1; i < n-1; i++) {
		for (j = 1; j < n-1; j++) {
			for (k = 1; k < n-1; k++) {
				A[i][j][k] = 0.125 * (B[i+1][j][k] - (2.0) * B[i][j][k] + B[i-1][j][k])
					+ 0.125 * (B[i][j+1][k] - (2.0) * B[i][j][k] + B[i][j-1][k])
					+ 0.125 * (B[i][j][k+1] - (2.0) * B[i][j][k] + B[i][j][k-1])
					+ B[i][j][k];
			}
		}
	}
    #pragma capc profitability_region end

    #pragma acc update self(A[0:n][0:n][0:n],B[0:n][0:n][0:n])

	printf("\nMatrix A :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			for(k=0;k<n;k++)
				printf("%f",A[i][j][k]);

	printf("\nMatrix B :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			for(k=0;k<n;k++)
				printf("%f ",B[i][j][k]);

	printf("\n");

    #pragma acc exit data delete(A[0:n][0:n][0:n],B[0:n][0:n][0:n])

	return 0;
}