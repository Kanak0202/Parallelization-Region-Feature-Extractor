#include<stdio.h>
#include<math.h>

#define N 13000

int SIZE;

float encrypt[N][1], a[N][N], b[N][N], mes[N][1], c[N][N];

void encryption();
void getKeyMessage();

int main()
{
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
        #pragma omp parallel for
        for(i = 0; i < SIZE; i++)
                mes[i][0] = msg[i] - 97;
        #pragma capc profitability_region end


        #pragma capc profitability_region begin
        #pragma omp parallel for collapse(2)
        for(i = 0; i < SIZE; i++)
                for(j = 0; j < SIZE; j++)
                {
                        a[i][j]=i+j+1+'0';
                        c[i][j] = a[i][j];
                }
        #pragma capc profitability_region end
}

void encryption()
{
        int i, j, k;

        #pragma capc profitability_region begin
        #pragma omp parallel for collapse(2) private(k)
        for(i = 0; i < SIZE; i++)
                for(j = 0; j < 1; j++)
                        for(k = 0; k < SIZE; k++)
                                encrypt[i][j] = encrypt[i][j] + a[i][k] * mes[k][j];
        #pragma capc profitability_region end

        printf("\nEncrypted string is: ");
        for(i = 0; i < SIZE; i++)
                printf("%c", (char)(fmod(encrypt[i][0], 26) + 97));

}