//Vector Arithmatic

#include <stdio.h>
#define SIZE 5

int main()
{
	double A[SIZE],B[SIZE],C[SIZE],D[SIZE],E[SIZE];
	int i = 0;

	//Array initialization
#pragma capc profitability_region begin
	for (i=0; i<SIZE; ++i)
	{
		A[i] = (double)i;
		B[i] = (double)(i+1);
	}
#pragma capc profitability_region end

	//C=A+B
#pragma capc profitability_region begin
	for (i=0; i<SIZE; ++i)
	{
		C[i]=A[i]+B[i];
	}
#pragma capc profitability_region end

	//Verify result
	for (i=0; i<SIZE; ++i)
	{
		if (C[i] !=A[i] + B[i])
		{
			printf("Add : Something didn't work correctly!\n");
			break;
		}
	}

	if (i == SIZE)
	{
		printf("Add : Everything seems to work fine! \n");
	}

	//D=A-B	
#pragma capc profitability_region begin
	for (i=0; i<SIZE; ++i)
	{
		D[i]=A[i]-B[i];
	}
#pragma capc profitability_region end

	//Verify result
	for (i=0; i<SIZE; ++i)
	{
		if (D[i] !=A[i] - B[i])
		{
			printf("Sub : Something didn't work correctly!\n");
			break;
		}
	}

	if (i == SIZE)
	{
		printf("Sub : Everything seems to work fine! \n");
	}

	//E=A*B
#pragma capc profitability_region begin
	for (i=0; i<SIZE; ++i)
	{
		E[i]=A[i]*B[i];
	}
#pragma capc profitability_region end

	//Verify result
	for (i=0; i<SIZE; ++i)
	{
		if (E[i] !=A[i] * B[i])
		{
			printf("Mult : Something didn't work correctly!\n");
			break;
		}
	}

	if (i == SIZE)
	{
		printf("Mult : Everything seems to work fine! \n");
	}

	return 0;
}
