#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[6]={0}; static unsigned long long __capc_rc[6]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (omp3) =====\n");
 for(q=0;q<6;q++) if(__capc_rc[q]) printf("Region %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",q,__capc_rt[q],__capc_rc[q],__capc_rt[q]/(double)__capc_rc[q]);
 for(q=0;q<0;q++) if(__capc_tc[q]){
   printf("%s transfer %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",__capc_tk[q],q,__capc_tt[q],__capc_tc[q],__capc_tt[q]/(double)__capc_tc[q]);
   if(__capc_tk[q][0]=='H'){h+=__capc_tt[q];hc+=__capc_tc[q];} else {d+=__capc_tt[q];dc+=__capc_tc[q];}
 }
 if(hc) printf("H2D summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",h,hc,h/(double)hc);
 if(dc) printf("D2H summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",d,dc,d/(double)dc);
 printf("=======================================\n");
}
/* end CAPC timing support */


#define W 4096
#define H 2160

float imgIn[W][H];
float imgOut[W][H];
float y1[W][H];
float y2[W][H];

float alpha = 0.25f;

void init_array()
{
    int i, j;

    for (i = 0; i < W; i++)
    {
        for (j = 0; j < H; j++)
        {
            imgIn[i][j] =
                (float)((313 * i + 991 * j) % 65536) / 65535.0f;
        }
    }
}

void kernel_deriche()
{
    int i, j;

    float xm1, tm1, ym1, ym2;
    float xp1, xp2;
    float tp1, tp2;
    float yp1, yp2;

    float k;
    float a1, a2, a3, a4;
    float a5, a6, a7, a8;
    float b1, b2;
    float c1, c2;

    k = (1.0f - expf(-alpha)) *
        (1.0f - expf(-alpha)) /
        (1.0f + 2.0f * alpha * expf(-alpha)
         - expf(2.0f * alpha));

    a1 = a5 = k;
    a2 = a6 = k * expf(-alpha) * (alpha - 1.0f);
    a3 = a7 = k * expf(-alpha) * (alpha + 1.0f);
    a4 = a8 = -k * expf(-2.0f * alpha);

    b1 = powf(2.0f, -alpha);
    b2 = -expf(-2.0f * alpha);

    c1 = c2 = 1.0f;

    /* -------------------------------------------------- */
    /* Region 0: Vertical forward recursive pass          */
    /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
#pragma omp parallel for private(i,j,ym1,ym2,xm1)
    for (i = 0; i < W; i++)
    {
        ym1 = 0.0f;
        ym2 = 0.0f;
        xm1 = 0.0f;

        for (j = 0; j < H; j++)
        {
            y1[i][j] =
                a1 * imgIn[i][j] +
                a2 * xm1 +
                b1 * ym1 +
                b2 * ym2;

            xm1 = imgIn[i][j];
            ym2 = ym1;
            ym1 = y1[i][j];
        }
    }
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end

    /* -------------------------------------------------- */
    /* Region 1: Vertical backward recursive pass         */
    /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_1=omp_get_wtime();
#pragma omp parallel for private(i,j,yp1,yp2,xp1,xp2)
    for (i = 0; i < W; i++)
    {
        yp1 = 0.0f;
        yp2 = 0.0f;
        xp1 = 0.0f;
        xp2 = 0.0f;

        for (j = H - 1; j >= 0; j--)
        {
            y2[i][j] =
                a3 * xp1 +
                a4 * xp2 +
                b1 * yp1 +
                b2 * yp2;

            xp2 = xp1;
            xp1 = imgIn[i][j];

            yp2 = yp1;
            yp1 = y2[i][j];
        }
    }
__capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
__capc_rc[1]++;
#pragma capc profitability_region end

    /* -------------------------------------------------- */
    /* Region 2: Combine first two passes                 */
    /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_2=omp_get_wtime();
#pragma omp parallel for private(i,j)
    for (i = 0; i < W; i++)
    {
#pragma omp parallel for private(j)
        for (j = 0; j < H; j++)
        {
            imgOut[i][j] =
                c1 * (y1[i][j] + y2[i][j]);
        }
    }
__capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
__capc_rc[2]++;
#pragma capc profitability_region end

    /* -------------------------------------------------- */
    /* Region 3: Horizontal forward recursive pass        */
    /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_3=omp_get_wtime();
#pragma omp parallel for private(j,i,tm1,ym1,ym2)
    for (j = 0; j < H; j++)
    {
        tm1 = 0.0f;
        ym1 = 0.0f;
        ym2 = 0.0f;

        for (i = 0; i < W; i++)
        {
            y1[i][j] =
                a5 * imgOut[i][j] +
                a6 * tm1 +
                b1 * ym1 +
                b2 * ym2;

            tm1 = imgOut[i][j];

            ym2 = ym1;
            ym1 = y1[i][j];
        }
    }
__capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
__capc_rc[3]++;
#pragma capc profitability_region end

    /* -------------------------------------------------- */
    /* Region 4: Horizontal backward recursive pass       */
    /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_4=omp_get_wtime();
#pragma omp parallel for private(j,i,tp1,tp2,yp1,yp2)
    for (j = 0; j < H; j++)
    {
        tp1 = 0.0f;
        tp2 = 0.0f;
        yp1 = 0.0f;
        yp2 = 0.0f;

        for (i = W - 1; i >= 0; i--)
        {
            y2[i][j] =
                a7 * tp1 +
                a8 * tp2 +
                b1 * yp1 +
                b2 * yp2;

            tp2 = tp1;
            tp1 = imgOut[i][j];

            yp2 = yp1;
            yp1 = y2[i][j];
        }
    }
__capc_rt[4]+=omp_get_wtime()-__capc_rs_4;
__capc_rc[4]++;
#pragma capc profitability_region end

    /* -------------------------------------------------- */
    /* Region 5: Final combination                        */
    /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_5=omp_get_wtime();
#pragma omp parallel for private(i,j)
    for (i = 0; i < W; i++)
    {
#pragma omp parallel for private(j)
        for (j = 0; j < H; j++)
        {
            imgOut[i][j] =
                c2 * (y1[i][j] + y2[i][j]);
        }
    }
__capc_rt[5]+=omp_get_wtime()-__capc_rs_5;
__capc_rc[5]++;
#pragma capc profitability_region end
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_deriche();

    /* Prevent complete dead-code elimination. */
    printf("%f\n", imgOut[0][0]);

    return 0;
}