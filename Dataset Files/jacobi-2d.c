//2D Jacobi Computation

#include<stdio.h>
#define n 5

int main()
{
	int i,j;

	double A[n][n],B[n][n];

	//Array initialization
	for(i=0;i<n;i++)
	{
		for(j=0;j<n;j++)
		{
			A[i][j]=(double)(0.1*i+j);
			B[i][j]=(double)(0.2*j+i);
			printf("");
		}
	}

	//Computations
    #pragma capc profitability_region begin
	for (i = 1; i < n; i++)
		for (j = 1; j < n - 1; j++)
			B[i][j] = 0.2 * (A[i][j] + A[i][j-1] + A[i][1+j] + A[1+i][j] + A[i-1][j]);

    #pragma capc profitability_region end

    #pragma capc profitability_region begin
	for (i = 1; i < n - 1; i++)
		for (j = 1; j < n - 1; j++)
			A[i][j] = 0.2 * (B[i][j] + B[i][j-1] + B[i][1+j] + B[1+i][j] + B[i-1][j]);

    #pragma capc profitability_region end

	printf("\nMatrix A :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			printf("%lf",A[i][j]);

	printf("\nMatrix B :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			printf("%lf ",B[i][j]);

	printf("\n");

	return 0;
}
