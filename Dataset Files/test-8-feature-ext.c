// Count Even Numbers with Reduction

#include<stdio.h>

#define N 1000

int main()
{
	int i;
	int a[N];
	int count;

	//Array Initialization
	for(i=0;i<N;i++)
	{
		a[i]=i;
	}

	count=0;

	//Count Even Numbers
    #pragma capc profitability_region begin
	for(i=0;i<N;i++)
	{
		if(a[i]%2==0)
		{
			count=count+1;
		}
	}
    #pragma capc profitability_region end

	printf("Number of Even Elements = %d\n",count);

	return 0;
}