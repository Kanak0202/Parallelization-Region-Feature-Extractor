#include <omp.h>
#include <stdio.h>

//3 Matrix Multiplications (E=A.B; F=C.D; G=E.F)

#include<stdio.h>

#define N 8000

int main()
{
	int i,j,k;
	double a[N][N],b[N][N],c[N][N],d[N][N],e[N][N],f[N][N],result[N][N];

    #pragma omp target enter data map(alloc:a[0:N][0:N],b[0:N][0:N],c[0:N][0:N],d[0:N][0:N],e[0:N][0:N],f[0:N][0:N],result[0:N][0:N])

	//Array Initialization
    #pragma capc profitability_region begin
{
  double _capc_t_start, _capc_t_end, _capc_tot;
  double _capc_k_sum = 0.0;
  double _capc_k0, _capc_k1;
  _capc_t_start = omp_get_wtime();
  _capc_k0 = omp_get_wtime();
    #pragma omp target teams distribute parallel for collapse(2) private(i,j)
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
  _capc_k1 = omp_get_wtime();
  _capc_k_sum += (_capc_k1 - _capc_k0);
  printf("[PROFILER] line:16 | Kernel Execution Time = %.9f s\n", _capc_k1 - _capc_k0);
  _capc_t_end = omp_get_wtime();
  _capc_tot = _capc_t_end - _capc_t_start;
  printf("[PROFILER] line:15 | Transfer Time = %.9f s\n", (_capc_tot - _capc_k_sum > 0.0 ? _capc_tot - _capc_k_sum : 0.0));
}
    #pragma capc profitability_region end

	//result = a.b
    #pragma capc profitability_region begin
{
  double _capc_t_start, _capc_t_end, _capc_tot;
  double _capc_k_sum = 0.0;
  double _capc_k0, _capc_k1;
  _capc_t_start = omp_get_wtime();
  _capc_k0 = omp_get_wtime();
    #pragma omp target teams distribute parallel for collapse(2) private(i,j,k)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+a[i][k]*b[k][j];
  _capc_k1 = omp_get_wtime();
  _capc_k_sum += (_capc_k1 - _capc_k0);
  printf("[PROFILER] line:34 | Kernel Execution Time = %.9f s\n", _capc_k1 - _capc_k0);
  _capc_t_end = omp_get_wtime();
  _capc_tot = _capc_t_end - _capc_t_start;
  printf("[PROFILER] line:33 | Transfer Time = %.9f s\n", (_capc_tot - _capc_k_sum > 0.0 ? _capc_tot - _capc_k_sum : 0.0));
}
    #pragma capc profitability_region end

	//print a.b

    #pragma omp target update from(result[0:N][0:N])
	
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
{
  double _capc_t_start, _capc_t_end, _capc_tot;
  double _capc_k_sum = 0.0;
  double _capc_k0, _capc_k1;
  _capc_t_start = omp_get_wtime();
  _capc_k0 = omp_get_wtime();
    #pragma omp target teams distribute parallel for collapse(2) private(i,j,k)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+c[i][k]*d[k][j];
  _capc_k1 = omp_get_wtime();
  _capc_k_sum += (_capc_k1 - _capc_k0);
  printf("[PROFILER] line:62 | Kernel Execution Time = %.9f s\n", _capc_k1 - _capc_k0);
  _capc_t_end = omp_get_wtime();
  _capc_tot = _capc_t_end - _capc_t_start;
  printf("[PROFILER] line:61 | Transfer Time = %.9f s\n", (_capc_tot - _capc_k_sum > 0.0 ? _capc_tot - _capc_k_sum : 0.0));
}
    #pragma capc profitability_region end

	//print c.d

    #pragma omp target update from(result[0:N][0:N])

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
{
  double _capc_t_start, _capc_t_end, _capc_tot;
  double _capc_k_sum = 0.0;
  double _capc_k0, _capc_k1;
  _capc_t_start = omp_get_wtime();
  _capc_k0 = omp_get_wtime();
    #pragma omp target teams distribute parallel for collapse(2) private(i,j,k)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+e[i][k]*f[k][j];
  _capc_k1 = omp_get_wtime();
  _capc_k_sum += (_capc_k1 - _capc_k0);
  printf("[PROFILER] line:90 | Kernel Execution Time = %.9f s\n", _capc_k1 - _capc_k0);
  _capc_t_end = omp_get_wtime();
  _capc_tot = _capc_t_end - _capc_t_start;
  printf("[PROFILER] line:89 | Transfer Time = %.9f s\n", (_capc_tot - _capc_k_sum > 0.0 ? _capc_tot - _capc_k_sum : 0.0));
}
    #pragma capc profitability_region end

	//print e.f

    #pragma omp target update from(result[0:N][0:N])

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

    #pragma omp target exit data map(delete:a[0:N][0:N],b[0:N][0:N],c[0:N][0:N],d[0:N][0:N],e[0:N][0:N],f[0:N][0:N],result[0:N][0:N])

	return 0;
}