// Integer Division with Arithmetic

#include<stdio.h>

#define N 1000

int main()
{
	int i;
	int a[N],b[N],result[N];

	//Array Initialization
	for(i=0;i<N;i++)
	{
		a[i]=i*100;
		b[i]=i+1;
	}

	//Integer Division
    #pragma capc profitability_region begin
	for(i=0;i<N;i++)
	{
		result[i]=(a[i]+100)/b[i];
	}
    #pragma capc profitability_region end

	printf("result[0] = %d\n",result[0]);
	printf("result[%d] = %d\n",N-1,result[N-1]);

	return 0;
}