#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>

#define N 49000000
#define T 500

double a[N];
double b[N];

void init_array()
{
        int i, j;
        #pragma capc profitability_region begin
        for (i=0; i<N; i++)
        {
                a[i] = ((double)i)/N;
                b[i] = ((double)i+1)/N;
        }
        #pragma capc profitability_region end
}

void print_array()
{
        int i, j;

        for (i=0; i<N; i++)
                printf("%lf ", a[i]);

        printf("\n");

        for (i=0; i<N; i++)
                printf("%lf ", b[i]);
}

int main()
{
        int t, i, j;

        init_array();

        for (t = 0; t < T; t++)
        {
                
                #pragma capc profitability_region begin
                for (i = 2; i < N - 1; i++)
                {
                        b[i] = 0.33333 * (a[i-1] + a[i] + a[i + 1]);
                }
                #pragma capc profitability_region end
                
                
                #pragma capc profitability_region begin
                for (i = 2; i < N - 1; i++)
                {
                        a[i] = b[i];
                }
                #pragma capc profitability_region end
                
        }

//      print_array();

       printf("a[0]=%lf\n",a[0]);
        printf("a[%d]=%lf\n",N-2,a[N-2]);

        printf("b[0]=%lf\n",b[0]);
        printf("b[%d]=%lf\n",N-1,b[N-1]);

        return 0;
}