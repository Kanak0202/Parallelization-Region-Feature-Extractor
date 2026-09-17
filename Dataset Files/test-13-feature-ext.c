// Integer and Floating Point Division

#include<stdio.h>

#define N 1000

int main()
{
	int i;
	int a[N],b[N],int_result[N];
	double x[N],y[N],float_result[N];

	//Array Initialization
	for(i=0;i<N;i++)
	{
		a[i]=i*100;
		b[i]=i+1;

		x[i]=(double)(i+1)*10.0;
		y[i]=(double)(i+2);
	}

	//Integer and Floating Point Division
    #pragma capc profitability_region begin
	for(i=0;i<N;i++)
	{
		int_result[i]=a[i]/b[i];
		float_result[i]=x[i]/y[i];
	}
    #pragma capc profitability_region end

	printf("int_result[0] = %d\n",int_result[0]);
	printf("float_result[0] = %lf\n",float_result[0]);

	return 0;
}