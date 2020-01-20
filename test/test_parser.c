#include "minicc.h"

static int test(const char *filename)
{
    PARSER *pars;
    int result;

    pars = open_parser(filename);
    if (pars == NULL) {
        fprintf(stderr, "couldn't open '%s'\n", filename);
        return 1;
    }
    result = parse(pars) ? 0 : 1;
    close_parser(pars);
    return result;
}

void usage()
{
    printf("usage: test_parser [-d[istp]] filename\n");
    printf(" -di  debug ident\n");
    printf(" -ds  debug scanner\n");
    printf(" -dt  debug parser_trace\n");
    printf(" -dp  debug parser\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    int i, j;
    int result = 0;

    if (argc < 2)
        usage();
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'd') {
                for (j = 2; argv[i][j] != 0; j++) {
                    switch (argv[i][j]) {
                    case 'i': set_debug("ident"); break;
                    case 's': set_debug("scanner"); break;
                    case 't': set_debug("parser_trace"); break;
                    case 'p': set_debug("parser"); break;
                    default: usage();
                    }
                }
            } else
                usage();
        } else if (test(argv[i]) != 0)
            result = 1;
    }
    return result;
}
