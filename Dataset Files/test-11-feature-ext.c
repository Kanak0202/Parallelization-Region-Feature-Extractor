// Floating Point Division

#include<stdio.h>

#define N 1000

int main()
{
	int i;
	double a[N],b[N],result[N];

	//Array Initialization
	for(i=0;i<N;i++)
	{
		a[i]=(double)(i+1)*10.0;
		b[i]=(double)(i+2);
	}

	//Floating Point Division
    #pragma capc profitability_region begin
	for(i=0;i<N;i++)
	{
		result[i]=a[i]/b[i];
	}
    #pragma capc profitability_region end

	printf("result[0] = %lf\n",result[0]);
	printf("result[%d] = %lf\n",N-1,result[N-1]);

	return 0;
}