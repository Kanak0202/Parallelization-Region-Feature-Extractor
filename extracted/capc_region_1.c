#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#define n 17

void capc_region_1(float (* restrict B)[17][17], float (* restrict A)[17][17])
{
    int i;
    int j;
    int k;
	for (i = 1; i < n-1; i++) {
		for (j = 1; j < n-1; j++) {
			for (k = 1; k < n-1; k++) {
				B[i][j][k] = 0.125 * (A[i+1][j][k] - (2.0) * A[i][j][k] + A[i-1][j][k])
					+ 0.125 * (A[i][j+1][k] - (2.0) * A[i][j][k] + A[i][j-1][k])
					+ 0.125 * (A[i][j][k+1] -(2.0) * A[i][j][k] + A[i][j][k-1])
					+ A[i][j][k];
			}
		}
	}

}
