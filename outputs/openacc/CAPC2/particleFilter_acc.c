
#include <stdio.h>

#define N 1000000

int main()
{
    static int x[N], y[N], z[N];
    static int distance[N];
    static int normalized[N];
    static int category[N];

    int i;

    // Initialize particle coordinates
    #pragma capc profitability_region begin
    #pragma acc parallel loop \
        copyout(x[0:N], y[0:N], z[0:N])
    for (i = 0; i < N; i++)
    {
        x[i] = (i * 17 + 11) % 10000;
        y[i] = (i * 23 + 19) % 10000;
        z[i] = (i * 31 + 29) % 10000;
    }
    #pragma capc profitability_region end

    // Compute a Manhattan-style distance
    #pragma capc profitability_region begin
    #pragma acc parallel loop \
        copyin(x[0:N], y[0:N], z[0:N]) \
        copyout(distance[0:N])
    for (i = 0; i < N; i++)
    {
        int ax = x[i];
        int ay = y[i];
        int az = z[i];

        distance[i] = ax + ay + az;
    }
    #pragma capc profitability_region end

    // Normalize distance
    #pragma capc profitability_region begin
    #pragma acc parallel loop \
        copyin(distance[0:N]) \
        copyout(normalized[0:N])
    for (i = 0; i < N; i++)
    {
        if (distance[i] > 20000)
            normalized[i] = distance[i] / 4;
        else if (distance[i] > 10000)
            normalized[i] = distance[i] / 3;
        else
            normalized[i] = distance[i] / 2;
    }
    #pragma capc profitability_region end

    // Classify particles
    #pragma capc profitability_region begin
    #pragma acc parallel loop \
        copyin(normalized[0:N]) \
        copyout(category[0:N])
    for (i = 0; i < N; i++)
    {
        if (normalized[i] > 10000)
            category[i] = 3;
        else if (normalized[i] > 5000)
            category[i] = 2;
        else if (normalized[i] > 1000)
            category[i] = 1;
        else
            category[i] = 0;
    }
    #pragma capc profitability_region end

    // Compute final particle feature
    #pragma capc profitability_region begin
    #pragma acc parallel loop \
        copyin(normalized[0:N], category[0:N]) \
        copyout(distance[0:N])
    for (i = 0; i < N; i++)
    {
        distance[i] = (normalized[i] + category[i]) / 2;
    }
    #pragma capc profitability_region end

    printf("Distance[0] = %d\n", distance[0]);
    printf("Category[0] = %d\n", category[0]);

    return 0;
}