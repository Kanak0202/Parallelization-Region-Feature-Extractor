#include<stdio.h>
#define N 50
//#define M 10
//#define K 10

int main()
{
	int i,j,k;
	//double a[N][M],b[M][K],c[N][K];
	double a[N][N],b[N][N],c[N][N];

	#pragma omp target enter data map(alloc:a[0:N][0:N],b[0:N][0:N],c[0:N][0:N])

	#pragma capc profitability_region begin
	#pragma omp target teams distribute parallel for collapse(2) map(alloc:a[0:N][0:N])
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			a[i][j]=i+1;
		}
	}
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
	#pragma omp target teams distribute parallel for collapse(2) map(alloc:b[0:N][0:N])
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			b[i][j]=j+1;
		}
	}
    #pragma capc profitability_region end

    #pragma capc profitability_region begin

	#pragma omp target teams distribute parallel for collapse(2) map(alloc:c[0:N][0:N])
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			c[i][j]=0;
		}
	}
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
	#pragma omp target teams distribute parallel for collapse(2) private(k) map(alloc:a[0:N][0:N],b[0:N][0:N],c[0:N][0:N])
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				c[i][j]= c[i][j]+a[i][k]*b[k][j];
    #pragma capc profitability_region end

	#pragma omp target update from(c[0:N][0:N])

    
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			printf("%lf\t",c[i][j]);
		}
		printf("\n");
	}

	#pragma omp target exit data map(delete:a[0:N][0:N],b[0:N][0:N],c[0:N][0:N])

	return 0;
}