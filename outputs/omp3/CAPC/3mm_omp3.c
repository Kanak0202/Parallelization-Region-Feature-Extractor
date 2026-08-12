//3 Matrix Multiplications (E=A.B; F=C.D; G=E.F)

#include<stdio.h>

#define N 5000

int main()
{
	int i,j,k;
	double a[N][N],b[N][N],c[N][N],d[N][N],e[N][N],f[N][N],result[N][N];

	//Array Initialization
    #pragma capc profitability_region begin
    #pragma omp parallel for collapse(2) private(i,j)
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			a[i][j]=(double)(0.1*i+j);	
			b[i][j]=(double)(0.2*j+i);
			c[i][j]=(double)(0.3*i+j);
			d[i][j]=(double)(0.4*j+i);
			e[i][j]=(double)(0.5*i+j);
			f[i][j]=(double)(0.6*j+i);
			result[i][j]=0.0; printf("");
		}
	}
    #pragma capc profitability_region end

	//result = a.b
    #pragma capc profitability_region begin
    #pragma omp parallel for collapse(2) private(i,j,k)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+a[i][k]*b[k][j];
    #pragma capc profitability_region end

	//print a.b
	
       printf("A[0][0]=%lf\n",result[0][0]);
       printf("A[%d][%d]=%lf\n",N-1,N-1,result[N-1][N-1]);
#if 0
	printf("\nResult=A.B\n");
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			printf("%lf\t",result[i][j]);
		}
		printf("\n");
	}

#endif
#if 1
	//result = c.d
    #pragma capc profitability_region begin
    #pragma omp parallel for collapse(2) private(i,j,k)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+c[i][k]*d[k][j];
    #pragma capc profitability_region end

	//print c.d

       printf("B[0][0]=%lf\n",result[0][0]);
       printf("B[%d][%d]=%lf\n",N-1,N-1,result[N-1][N-1]);

#if 0

	printf("\nResult=C.D\n");
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			printf("%lf\t",result[i][j]);
		}
		printf("\n");
	}
#endif
	//result = e.f
    #pragma capc profitability_region begin
    #pragma omp parallel for collapse(2) private(i,j,k)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+e[i][k]*f[k][j];
    #pragma capc profitability_region end

	//print e.f
       printf("C[0][0]=%lf\n",result[0][0]);
       printf("C[%d][%d]=%lf\n",N-1,N-1,result[N-1][N-1]);
   
 #endif

#if 0	
	printf("\nResult=E.F\n");
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			printf("%lf\t",result[i][j]);
		}
		printf("\n");
	}
#endif
	return 0;
}