/*
#include <stdio.h>
void print_num(int n)
{
    printf("%d\n", n);
}
void print_num(int n);
*/

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

/*
int main()
{
    int x;
    x = fact(6);
    print_num(x);
    return 0;
}
*/
