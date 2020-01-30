void putc(char c);
void print(char *s)
{
    int c;
    while ((c = *s++) != 0)
        putc(c);
}

int main()
{
    print("hello world\n");
    return 0;
}
