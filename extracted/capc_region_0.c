#include<stdio.h>
#define N 10

void capc_region_0(double (* restrict a)[10], double (* restrict b)[10], double (* restrict c)[10], double (* restrict d)[10], double (* restrict e)[10], double (* restrict f)[10], double (* restrict result)[10])
{
    int i;
    int j;
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
			result[i][j]=0.0;
		}
	}

}
