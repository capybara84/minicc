#include "minicc.h"

static int test(const char *filename)
{
    PARSER *pars;
    int result;

    pars = open_parser(filename);
    if (pars == NULL) {
        fprintf(stderr, "open_parser(%s) failed\n", filename);
        return 1;
    }
    result = parse(pars) ? 0 : 1;
    close_parser(pars);
    return result;
}

int main(int argc, char *argv[])
{
    int i;
    int result = 0;

    if (argc < 2) {
        printf("usage: test_parser [-ds] filename\n");
        return 1;
    }
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-ds") == 0)
            set_debug("scanner");
        else if (strcmp(argv[i], "-dp") == 0)
            set_debug("parser");
        else
            result += test(argv[i]);
    }
    return result;
}
