#include <stdio.h>

int fact(int n);

/*
int fact(int n)
{
    int m, f;
    m = 1;
    f = 1;
    while (m <= n) {
        f = f * m;
        m = m + 1;
    }
    return f;
}
*/

int main()
{
    int i;
    for (i = 0; i < 100; i++)
        printf("fact(%d) = %d\n", i, fact(i));
    return 0;
}
