// Array Sum with Reduction

#include<stdio.h>

#define N 1000

int main()
{
	int i;
	double a[N],sum;

	//Array Initialization
	for(i=0;i<N;i++)
	{
		a[i]=(double)(0.5*i);
	}

	sum=0.0;

	//Sum of array elements
    #pragma capc profitability_region begin
	for(i=0;i<N;i++)
	{
		sum=sum+a[i];
	}
    #pragma capc profitability_region end

	printf("Sum = %lf\n",sum);

	return 0;
}