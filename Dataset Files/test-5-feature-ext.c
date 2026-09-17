// Dot Product with Reduction

#include<stdio.h>

#define N 1000

int main()
{
	int i;
	double a[N],b[N],dot;

	//Array Initialization
	for(i=0;i<N;i++)
	{
		a[i]=(double)(0.5*i);
		b[i]=(double)(0.25*i);
	}

	dot=0.0;

	//Dot Product
    #pragma capc profitability_region begin
	for(i=0;i<N;i++)
	{
		dot=dot+a[i]*b[i];
	}
    #pragma capc profitability_region end

	printf("Dot Product = %lf\n",dot);

	return 0;
}