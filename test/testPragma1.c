int main()
{
#pragma omp parallel
    {
    }

#pragma capc profitability_region begin

    for(int i=0;i<10;i++)
    {
    }

#pragma capc profitability_region end

    return 0;
}
