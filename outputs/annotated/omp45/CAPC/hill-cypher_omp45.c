#include<stdio.h>
#include<math.h>

/* ---- capc timing instrumentation: globals ---- */
#include <omp.h>
static double __capc_region_0_total = 0.0;
static long   __capc_region_0_count = 0;
static double __capc_region_1_total = 0.0;
static long   __capc_region_1_count = 0;
static double __capc_region_2_total = 0.0;
static long   __capc_region_2_count = 0;
static double __capc_h2d_msg = -1.0;
static double __capc_d2h_msg = -1.0;
static double __capc_h2d_mes = -1.0;
static double __capc_d2h_mes = -1.0;
static double __capc_h2d_a = -1.0;
static double __capc_d2h_a = -1.0;
static double __capc_h2d_c = -1.0;
static double __capc_d2h_c = -1.0;
static double __capc_h2d_encrypt = -1.0;
static double __capc_d2h_encrypt = -1.0;
/* ---- end globals ---- */


#define N 13000

int SIZE;

float encrypt[N][1], a[N][N], b[N][N], mes[N][1], c[N][N];

void encryption();
void getKeyMessage();

int main()
{
        getKeyMessage();
        encryption();

/* ---- capc timing instrumentation: report ---- */
{
  double __resident = (__capc_region_0_count > 0) ? (__capc_region_0_total / __capc_region_0_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_msg > 0) __xfer += __capc_h2d_msg;
  if (__capc_d2h_msg > 0) __xfer += __capc_d2h_msg;
  if (__capc_h2d_mes > 0) __xfer += __capc_h2d_mes;
  if (__capc_d2h_mes > 0) __xfer += __capc_d2h_mes;
  printf("region_0 (pragma at original line 53): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_0_count);
  printf("    msg: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_msg > 0 ? __capc_h2d_msg : 0.0, __capc_d2h_msg > 0 ? __capc_d2h_msg : 0.0);
  printf("    mes: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_mes > 0 ? __capc_h2d_mes : 0.0, __capc_d2h_mes > 0 ? __capc_d2h_mes : 0.0);
}
{
  double __resident = (__capc_region_1_count > 0) ? (__capc_region_1_total / __capc_region_1_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_a > 0) __xfer += __capc_h2d_a;
  if (__capc_d2h_a > 0) __xfer += __capc_d2h_a;
  if (__capc_h2d_c > 0) __xfer += __capc_h2d_c;
  if (__capc_d2h_c > 0) __xfer += __capc_d2h_c;
  printf("region_1 (pragma at original line 60): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_1_count);
  printf("    a: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_a > 0 ? __capc_h2d_a : 0.0, __capc_d2h_a > 0 ? __capc_d2h_a : 0.0);
  printf("    c: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_c > 0 ? __capc_h2d_c : 0.0, __capc_d2h_c > 0 ? __capc_d2h_c : 0.0);
}
{
  double __resident = (__capc_region_2_count > 0) ? (__capc_region_2_total / __capc_region_2_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_a > 0) __xfer += __capc_h2d_a;
  if (__capc_d2h_a > 0) __xfer += __capc_d2h_a;
  if (__capc_h2d_mes > 0) __xfer += __capc_h2d_mes;
  if (__capc_d2h_mes > 0) __xfer += __capc_d2h_mes;
  if (__capc_h2d_encrypt > 0) __xfer += __capc_h2d_encrypt;
  if (__capc_d2h_encrypt > 0) __xfer += __capc_d2h_encrypt;
  printf("region_2 (pragma at original line 75): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_2_count);
  printf("    a: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_a > 0 ? __capc_h2d_a : 0.0, __capc_d2h_a > 0 ? __capc_d2h_a : 0.0);
  printf("    mes: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_mes > 0 ? __capc_h2d_mes : 0.0, __capc_d2h_mes > 0 ? __capc_d2h_mes : 0.0);
  printf("    encrypt: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_encrypt > 0 ? __capc_h2d_encrypt : 0.0, __capc_d2h_encrypt > 0 ? __capc_d2h_encrypt : 0.0);
}
/* ---- end report ---- */

        return 0;
}

void getKeyMessage()
{
        int i, j;
        char msg[N];

        SIZE=0;

        FILE *fptr;

        char ch,filename[15]="file.txt";

        fptr = fopen(filename, "r");

        if (fptr == NULL)
        {
                printf("Cannot open file \n");
                return;
        }

        ch = fgetc(fptr);
        while (ch != EOF)
        {
                msg[SIZE++]=ch;
                ch = fgetc(fptr);
        }

        fclose(fptr);

        printf("\nOriginal string");
        for(i = 0; i < SIZE; i++)
                printf("%c",msg[i]);

        #pragma capc profitability_region begin
{ double __capc_t0 = omp_get_wtime();
        #pragma omp target teams distribute parallel for map(to:msg[0:SIZE]) map(from:mes[0:SIZE][0:1])
        for(i = 0; i < SIZE; i++)
                mes[i][0] = msg[i] - 97;
double __capc_t1 = omp_get_wtime(); __capc_region_0_total += (__capc_t1 - __capc_t0); __capc_region_0_count += 1; }
        #pragma capc profitability_region end


        #pragma capc profitability_region begin
{ double __capc_t0 = omp_get_wtime();
        #pragma omp target teams distribute parallel for collapse(2) map(from:a[0:SIZE][0:SIZE],c[0:SIZE][0:SIZE])
        for(i = 0; i < SIZE; i++)
                for(j = 0; j < SIZE; j++)
                {
                        a[i][j]=i+j+1+'0';
                        c[i][j] = a[i][j];
                }
double __capc_t1 = omp_get_wtime(); __capc_region_1_total += (__capc_t1 - __capc_t0); __capc_region_1_count += 1; }
        #pragma capc profitability_region end
}

void encryption()
{
        int i, j, k;

        #pragma capc profitability_region begin
{ double __capc_t0 = omp_get_wtime();
        #pragma omp target teams distribute parallel for collapse(2) private(k) map(to:a[0:SIZE][0:SIZE],mes[0:SIZE][0:1]) map(tofrom:encrypt[0:SIZE][0:1])
        for(i = 0; i < SIZE; i++)
                for(j = 0; j < 1; j++)
                        for(k = 0; k < SIZE; k++)
                                encrypt[i][j] = encrypt[i][j] + a[i][k] * mes[k][j];
double __capc_t1 = omp_get_wtime(); __capc_region_2_total += (__capc_t1 - __capc_t0); __capc_region_2_count += 1; }
        #pragma capc profitability_region end

        printf("\nEncrypted string is: ");
        for(i = 0; i < SIZE; i++)
                printf("%c", (char)(fmod(encrypt[i][0], 26) + 97));

}