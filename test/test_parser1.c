/* external declaration */
int i,j;
char c;
int a[10];
unsigned long *p;
void foo(void);
int main(int argc, char *argv[]);

int const *c0;
int *const c1;
int const * const c2;

int *a0[10];
int (*a1)[10];

/* storage class */
static unsigned short sca;
extern double scb;

scfoo() {}
scbar();

const volatile int cvi;
volatile const int vci;
const const int cci;
volatile volatile int vvi;
volatile const volatile int vcvi;
