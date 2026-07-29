#include <stdio.h>

int main()
{
#pragma capc profitability_region begin

    for (int i = 0; i < 100; i++)
    {
        printf("%d\n", i);
    }

#pragma capc profitability_region end

    return 0;
}
