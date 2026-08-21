#include <stdio.h>

#define N 10000000
#define ITER 20

double A[N];
double B[N];
double C[N];

int main()
{
    int i, t;

    /* Host initialization */
    for (i = 0; i < N; i++)
    {
        A[i] = (double)i;
        B[i] = (double)(2 * i);
        C[i] = 0.0;
    }

    /*
     * Allocate persistent device storage only.
     * No initial transfer here.
     */
    #pragma omp target enter data map(alloc:A[0:N], B[0:N], C[0:N])

    for (t = 0; t < ITER; t++)
    {
        /*
         * =====================================================
         * Region 1
         *
         * Explicit recurring H2D before the region.
         *
         * A,B = inputs
         * C   = output
         * =====================================================
         */
        #pragma omp target update to(A[0:N], B[0:N])

        #pragma capc profitability_region begin
        #pragma omp target teams distribute parallel for map(alloc:A[0:N], B[0:N], C[0:N])
        for (i = 0; i < N; i++)
        {
            C[i] = A[i] + B[i];
        }
        #pragma capc profitability_region end

        #pragma omp target update from(C[0:N])


        /*
         * =====================================================
         * Region 2
         *
         * No explicit transfer around this region.
         * C is already on GPU from Region 1.
         *
         * C = read/write
         * =====================================================
         */
        #pragma capc profitability_region begin
        #pragma omp target teams distribute parallel for map(alloc:C[0:N])
        for (i = 0; i < N; i++)
        {
            C[i] = 2.5 * C[i];
        }
        #pragma capc profitability_region end
    }

    #pragma omp target update from(C[0:N])

    printf("C[0] = %f\n", C[0]);
    printf("C[N-1] = %f\n", C[N-1]);

    #pragma omp target exit data map(delete:A[0:N], B[0:N], C[0:N])

    return 0;
}