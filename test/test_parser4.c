int foo0(void);
int foo1(int a);
int foo2(int a, int b);
int foo3(int a, int b, int c);
int foo4(int a, int b, int c, int d);
int foo5(int a, int b, int c, int d, int e);
int foo6(int a, int b, int c, int d, int e, int f);
int foo7(int a, int b, int c, int d, int e, int f, int g);
int foo8(int a, int b, int c, int d, int e, int f, int g, int h);
int main()
{
    foo0();
    foo1(1);
    foo2(1,2);
    foo3(1,2,3);
    foo4(1,2,3,4);
    foo5(1,2,3,4,5);
    foo6(1,2,3,4,5,6);
    foo7(1,2,3,4,5,6,7);
    foo8(1,2,3,4,5,6,7,8);
    return 0;
}
