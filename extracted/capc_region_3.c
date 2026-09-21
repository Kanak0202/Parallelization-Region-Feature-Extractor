#include<stdio.h>
#define N 10

void capc_region_3(double (* restrict result)[10], double (* restrict e)[10], double (* restrict f)[10])
{
    int i;
    int j;
    int k;
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+e[i][k]*f[k][j];

}
