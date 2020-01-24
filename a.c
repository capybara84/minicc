int add(int a, int b)
{
    int x, y;

    a = !a;
    b = *(++p);
    x = (a + b) * 2, y = a - b;
    if (x == y)
        return a + b;
    return a - b;
}
