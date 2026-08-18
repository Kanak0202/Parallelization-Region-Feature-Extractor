#include<stdio.h>
#include<math.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[3]={0}; static unsigned long long __capc_rc[3]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (serial) =====\n");
 for(q=0;q<3;q++) if(__capc_rc[q]) printf("Region %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",q,__capc_rt[q],__capc_rc[q],__capc_rt[q]/(double)__capc_rc[q]);
 for(q=0;q<0;q++) if(__capc_tc[q]){
   printf("%s transfer %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",__capc_tk[q],q,__capc_tt[q],__capc_tc[q],__capc_tt[q]/(double)__capc_tc[q]);
   if(__capc_tk[q][0]=='H'){h+=__capc_tt[q];hc+=__capc_tc[q];} else {d+=__capc_tt[q];dc+=__capc_tc[q];}
 }
 if(hc) printf("H2D summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",h,hc,h/(double)hc);
 if(dc) printf("D2H summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",d,dc,d/(double)dc);
 printf("=======================================\n");
}
/* end CAPC timing support */


#define N 13000

int SIZE;

float encrypt[N][1], a[N][N], b[N][N], mes[N][1], c[N][N];

void encryption();
void getKeyMessage();

int main()
{
    atexit(__capc_report);
        getKeyMessage();
        encryption();
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
        double __capc_rs_0=omp_get_wtime();
        for(i = 0; i < SIZE; i++)
                mes[i][0] = msg[i] - 97;
        __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
        __capc_rc[0]++;
        #pragma capc profitability_region end


        #pragma capc profitability_region begin
        double __capc_rs_1=omp_get_wtime();
        for(i = 0; i < SIZE; i++)
                for(j = 0; j < SIZE; j++)
                {
                        a[i][j]=i+j+1+'0';
                        c[i][j] = a[i][j];
                }
        __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
        __capc_rc[1]++;
        #pragma capc profitability_region end
}

void encryption()
{
        int i, j, k;

        #pragma capc profitability_region begin
        double __capc_rs_2=omp_get_wtime();
        for(i = 0; i < SIZE; i++)
                for(j = 0; j < 1; j++)
                        for(k = 0; k < SIZE; k++)
                                encrypt[i][j] = encrypt[i][j] + a[i][k] * mes[k][j];
        __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
        __capc_rc[2]++;
        #pragma capc profitability_region end

        printf("\nEncrypted string is: ");
        for(i = 0; i < SIZE; i++)
                printf("%c", (char)(fmod(encrypt[i][0], 26) + 97));

}