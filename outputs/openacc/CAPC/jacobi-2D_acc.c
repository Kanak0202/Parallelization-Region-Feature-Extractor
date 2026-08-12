//2D Jacobi Computation

#include<stdio.h>
#define n 5

int main()
{
	int i,j;

	double A[n][n],B[n][n];

	//Array initialization
    #pragma capc profitability_region begin
    #pragma omp parallel for collapse(2) private(i,j)
	for(i=0;i<n;i++)
	{
		for(j=0;j<n;j++)
		{
			A[i][j]=(double)(0.1*i+j);
			B[i][j]=(double)(0.2*j+i);
			printf("");
		}
	}
    #pragma capc profitability_region end

    #pragma acc enter data copyin(A[0:n][0:n],B[0:n][0:n])

	//Computations
    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(A[0:n][0:n],B[0:n][0:n])
	for (i = 1; i < n-1; i++)
		for (j = 1; j < n - 1; j++)
			B[i][j] = 0.2 * (A[i][j] + A[i][j-1] + A[i][1+j] + A[1+i][j] + A[i-1][j]);

    #pragma capc profitability_region end

    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(A[0:n][0:n],B[0:n][0:n])
	for (i = 1; i < n - 1; i++)
		for (j = 1; j < n - 1; j++)
			A[i][j] = 0.2 * (B[i][j] + B[i][j-1] + B[i][1+j] + B[1+i][j] + B[i-1][j]);

    #pragma capc profitability_region end

    #pragma acc update self(A[0:n][0:n],B[0:n][0:n])

	printf("\nMatrix A :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			printf("%lf",A[i][j]);

	printf("\nMatrix B :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			printf("%lf ",B[i][j]);

	printf("\n");

    #pragma acc exit data delete(A[0:n][0:n],B[0:n][0:n])

	return 0;
}