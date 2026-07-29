int add(int a, int b)
{
    return a + b;
}

int main()
{
    int x = add(5, 10);

    for (int i = 0; i < 10; i++)
        x += i;

    return x;
}
