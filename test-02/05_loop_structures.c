#include <stdio.h>

#define N 1024

int A[N];
int B[N];

void initialize()
{
    for (int i = 0; i < N; i++)
    {
        A[i] = i;
        B[i] = 0;
    }
}

/*----------------------------------------------------------*/
/* Simple Increasing Loop */
/*----------------------------------------------------------*/
void simple_loop()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        B[i] = A[i] + 1;
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Decrementing Loop */
/*----------------------------------------------------------*/
void decrement_loop()
{
#pragma capc profitability_region begin
    for (int i = N - 1; i >= 0; i--)
    {
        B[i] = A[i] + 2;
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Step Size = 2 */
/*----------------------------------------------------------*/
void step_two_loop()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i += 2)
    {
        B[i] = A[i] * 2;
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Nested Loop */
/*----------------------------------------------------------*/
void nested_loop()
{
#pragma capc profitability_region begin
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            B[i * 32 + j] = A[i * 32 + j] + 1;
        }
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Triple Nested Loop */
/*----------------------------------------------------------*/
void triple_nested_loop()
{
#pragma capc profitability_region begin
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            for (int k = 0; k < 16; k++)
            {
                int idx = i * 128 + j * 16 + k;
                B[idx] = A[idx] + 5;
            }
        }
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Loop With Conditional */
/*----------------------------------------------------------*/
void conditional_loop()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        if (A[i] & 1)
            B[i] = A[i];
        else
            B[i] = -A[i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Loop With Continue */
/*----------------------------------------------------------*/
void continue_loop()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        if (A[i] < 100)
            continue;

        B[i] = A[i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Empty Loop Body */
/*----------------------------------------------------------*/
void empty_loop()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
    }
#pragma capc profitability_region end
}

int main()
{
    initialize();

    simple_loop();
    decrement_loop();
    step_two_loop();
    nested_loop();
    triple_nested_loop();
    conditional_loop();
    continue_loop();
    empty_loop();

    printf("%d\n", B[0]);

    return 0;
}