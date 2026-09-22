#include<stdio.h>
#define N 600000000

void capc_region_3()
{
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+e[i][k]*f[k][j];

}
