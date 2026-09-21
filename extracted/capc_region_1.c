#include<stdio.h>
#define N 10

void capc_region_1(double (* restrict result)[10], double (* restrict a)[10], double (* restrict b)[10])
{
    int i;
    int j;
    int k;
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+a[i][k]*b[k][j];

}
