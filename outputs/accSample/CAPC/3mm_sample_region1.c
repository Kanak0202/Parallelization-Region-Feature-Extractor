// 3mm - Region 1 only

#include <stdio.h>

#define N 8000

int main()
{
    int i, j, k;

    double a[N][N],
           b[N][N],
           c[N][N],
           d[N][N],
           e[N][N],
           f[N][N],
           result[N][N];

    #pragma acc enter data create( \
        a[0:N][0:N], \
        b[0:N][0:N], \
        c[0:N][0:N], \
        d[0:N][0:N], \
        e[0:N][0:N], \
        f[0:N][0:N], \
        result[0:N][0:N])

    // =========================================================
    // REGION 1 ONLY
    // Array Initialization
    // =========================================================

    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(a,b,c,d,e,f,result)
    for(i = 0; i < N; i++)
    {
        for(j = 0; j < N; j++)
        {
            a[i][j] = (double)(0.1 * i + j);
            b[i][j] = (double)(0.2 * j + i);
            c[i][j] = (double)(0.3 * i + j);
            d[i][j] = (double)(0.4 * j + i);
            e[i][j] = (double)(0.5 * i + j);
            f[i][j] = (double)(0.6 * j + i);
            result[i][j] = 0.0;

            printf("");
        }
    }
    #pragma capc profitability_region end

    #pragma acc exit data delete( \
        a[0:N][0:N], \
        b[0:N][0:N], \
        c[0:N][0:N], \
        d[0:N][0:N], \
        e[0:N][0:N], \
        f[0:N][0:N], \
        result[0:N][0:N])

    return 0;
}