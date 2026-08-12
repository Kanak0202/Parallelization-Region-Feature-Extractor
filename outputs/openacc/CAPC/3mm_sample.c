//3 Matrix Multiplications (E=A.B; F=C.D; G=E.F)
#include<stdio.h>
#define N 2000

int main()
{
  int i;
  int j;
  int k;
  double a[2000][2000];
  double b[2000][2000];
  double c[2000][2000];
  double d[2000][2000];
  double e[2000][2000];
  double f[2000][2000];
  double result[2000][2000];
//Array Initialization
  for (i = 0; i <= 1999; i += 1) {
    for (j = 0; j <= 1999; j += 1) {
      a[i][j] = ((double )(0.1 * i + j));
      b[i][j] = ((double )(0.2 * j + i));
      c[i][j] = ((double )(0.3 * i + j));
      d[i][j] = ((double )(0.4 * j + i));
      e[i][j] = ((double )(0.5 * i + j));
      f[i][j] = ((double )(0.6 * j + i));
      result[i][j] = 0.0;
      printf("");
    }
  }
//result = a.b

#pragma acc parallel loop auto gang vector num_gangs(8) vector_length(256)
  for (i = 0; i <= 1999; i += 1) {
    for (j = 0; j <= 1999; j += 1) {
      for (k = 0; k <= 1999; k += 1) {
        result[i][j] = result[i][j] + a[i][k] * b[k][j];
      }
    }
  }
//print a.b
  printf("A[0][0]=%lf\n",result[0][0]);
  printf("A[%d][%d]=%lf\n",2000 - 1,2000 - 1,result[2000 - 1][2000 - 1]);
#if 0
#endif
#if 1
//result = c.d

#pragma acc parallel loop auto gang vector num_gangs(8) vector_length(256)
  for (i = 0; i <= 1999; i += 1) {
    for (j = 0; j <= 1999; j += 1) {
      for (k = 0; k <= 1999; k += 1) {
        result[i][j] = result[i][j] + c[i][k] * d[k][j];
      }
    }
  }
//print c.d
  printf("B[0][0]=%lf\n",result[0][0]);
  printf("B[%d][%d]=%lf\n",2000 - 1,2000 - 1,result[2000 - 1][2000 - 1]);
#if 0
#endif
//result = e.f

#pragma acc parallel loop auto gang vector num_gangs(8) vector_length(256)
  for (i = 0; i <= 1999; i += 1) {
    for (j = 0; j <= 1999; j += 1) {
      for (k = 0; k <= 1999; k += 1) {
        result[i][j] = result[i][j] + e[i][k] * f[k][j];
      }
    }
  }
//print e.f
  printf("C[0][0]=%lf\n",result[0][0]);
  printf("C[%d][%d]=%lf\n",2000 - 1,2000 - 1,result[2000 - 1][2000 - 1]);
 #endif
#if 0
#endif
  return 0;
}

