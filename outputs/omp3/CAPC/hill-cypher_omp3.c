// Hill Cipher - OpenMP 3.0 Version
// No user/file input
// Message and key matrix are initialized internally

#include <stdio.h>
#include <math.h>

#define N 100

int main()
{
	int i, j, k;

	float encrypt[N][1];
	float a[N][N];
	float mes[N][1];

	/* ---------------------------------------------------------
	   Message Initialization
	   --------------------------------------------------------- */

	#pragma capc profitability_region begin
	#pragma omp parallel for private(i)
	for(i = 0; i < N; i++)
	{
		mes[i][0] = (float)(i % 26);
		encrypt[i][0] = 0.0f;
	}
	#pragma capc profitability_region end


	/* ---------------------------------------------------------
	   Key Matrix Initialization
	   --------------------------------------------------------- */

	#pragma capc profitability_region begin
	#pragma omp parallel for private(i,j)
	for(i = 0; i < N; i++)
	{
		#pragma omp parallel for private(j)
		for(j = 0; j < N; j++)
		{
			a[i][j] = (float)(i + j + 1 + '0');
		}
	}
	#pragma capc profitability_region end


	/* ---------------------------------------------------------
	   Hill Cipher Encryption

	   encrypt = a * mes
	   --------------------------------------------------------- */

	#pragma capc profitability_region begin
	#pragma omp parallel for private(i,j,k)
	for(i = 0; i < N; i++)
	{
		#pragma omp parallel for private(j,k)
		for(j = 0; j < 1; j++)
		{
			for(k = 0; k < N; k++)
			{
				encrypt[i][j] =
					encrypt[i][j] +
					a[i][k] * mes[k][j];
			}
		}
	}
	#pragma capc profitability_region end


	printf("encrypt[0][0] = %f\n", encrypt[0][0]);
	printf("encrypt[%d][0] = %f\n", N-1, encrypt[N-1][0]);

	printf("Encrypted characters: ");
	printf("%c ", (char)(fmod(encrypt[0][0], 26.0f) + 97));
	printf("%c\n", (char)(fmod(encrypt[N-1][0], 26.0f) + 97));

	return 0;
}