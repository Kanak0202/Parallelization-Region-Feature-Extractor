// Sum of Squares with Reduction

#include<stdio.h>

#define N 1000

int main()
{
	int i;
	double a[N],sum_sq;

	//Array Initialization
	for(i=0;i<N;i++)
	{
		a[i]=(double)(0.5*i);
	}

	sum_sq=0.0;

	//Sum of Squares
    #pragma capc profitability_region begin
	for(i=0;i<N;i++)
	{
		sum_sq=sum_sq+a[i]*a[i];
	}
    #pragma capc profitability_region end

	printf("Sum of Squares = %lf\n",sum_sq);

	return 0;
}