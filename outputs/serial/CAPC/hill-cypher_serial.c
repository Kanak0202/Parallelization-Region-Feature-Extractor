// Hill Cipher - Serial Version
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

	   Equivalent to:
	       mes[i][0] = msg[i] - 97;

	   We generate characters cyclically:
	       a, b, c, ..., z, a, b, ...
	   Therefore mes contains:
	       0, 1, 2, ..., 25, 0, 1, ...
	   --------------------------------------------------------- */

	#pragma capc profitability_region begin
	for(i = 0; i < N; i++)
	{
		mes[i][0] = (float)(i % 26);
		encrypt[i][0] = 0.0f;
	}
	#pragma capc profitability_region end


	/* ---------------------------------------------------------
	   Key Matrix Initialization

	   Original code used:
	       a[i][j] = i + j + 1 + '0';

	   '0' has ASCII value 48, therefore:
	       a[i][j] = i + j + 49
	   --------------------------------------------------------- */

	#pragma capc profitability_region begin
	for(i = 0; i < N; i++)
	{
		for(j = 0; j < N; j++)
		{
			a[i][j] = (float)(i + j + 1 + '0');
		}
	}
	#pragma capc profitability_region end


	/* ---------------------------------------------------------
	   Hill Cipher Encryption

	   encrypt = a * mes

	   Original loop structure is preserved.
	   --------------------------------------------------------- */

	#pragma capc profitability_region begin
	for(i = 0; i < N; i++)
	{
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


	/* Print only a few values so that the compiler cannot
	   completely eliminate the computation. */

	printf("encrypt[0][0] = %f\n", encrypt[0][0]);
	printf("encrypt[%d][0] = %f\n", N-1, encrypt[N-1][0]);

	printf("Encrypted characters: ");
	printf("%c ", (char)(fmod(encrypt[0][0], 26.0f) + 97));
	printf("%c\n", (char)(fmod(encrypt[N-1][0], 26.0f) + 97));

	return 0;
}
