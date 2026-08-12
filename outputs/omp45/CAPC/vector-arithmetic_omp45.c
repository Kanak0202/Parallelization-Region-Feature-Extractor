//Vector Arithmatic

#include <stdio.h>
#define SIZE 5

int main()
{
	double A[SIZE],B[SIZE],C[SIZE],D[SIZE],E[SIZE];
	int i = 0;

#pragma omp target enter data map(alloc:A[0:SIZE],B[0:SIZE],C[0:SIZE],D[0:SIZE],E[0:SIZE])

	//Array initialization
#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(alloc:A[0:SIZE],B[0:SIZE])
	for (i=0; i<SIZE; ++i)
	{
		A[i] = (double)i;
		B[i] = (double)(i+1);
	}
#pragma capc profitability_region end

	//C=A+B
#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(alloc:A[0:SIZE],B[0:SIZE],C[0:SIZE])
	for (i=0; i<SIZE; ++i)
	{
		C[i]=A[i]+B[i];
	}
#pragma capc profitability_region end

#pragma omp target update from(A[0:SIZE],B[0:SIZE],C[0:SIZE])

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
#pragma omp target teams distribute parallel for map(alloc:A[0:SIZE],B[0:SIZE],D[0:SIZE])
	for (i=0; i<SIZE; ++i)
	{
		D[i]=A[i]-B[i];
	}
#pragma capc profitability_region end

#pragma omp target update from(D[0:SIZE])

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
#pragma omp target teams distribute parallel for map(alloc:A[0:SIZE],B[0:SIZE],E[0:SIZE])
	for (i=0; i<SIZE; ++i)
	{
		E[i]=A[i]*B[i];
	}
#pragma capc profitability_region end

#pragma omp target update from(E[0:SIZE])

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

#pragma omp target exit data map(delete:A[0:SIZE],B[0:SIZE],C[0:SIZE],D[0:SIZE],E[0:SIZE])

	return 0;
}