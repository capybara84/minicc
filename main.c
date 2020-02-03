#include "minicc.h"

#define MAX_PATH    256

static void change_filename_ext(char *name, const char *orig, const char *ext)
{
    char *p;
    strcpy(name, orig);
    p = strchr(name, '.');
    if (p)
        strcpy(p, ext);
    else
        strcat(p, ext);
}

static bool compile(const char *filename)
{
    char out_name[MAX_PATH+1];
    PARSER *pars;
    bool result;

    pars = open_parser(filename);
    if (pars == NULL) {
        fprintf(stderr, "couldn't open '%s'\n", filename);
        return false;
    }
    result = parse(pars);
    close_parser(pars);

    print_global_symtab();

    if (result) {
        FILE *fp;
        change_filename_ext(out_name, filename, ".s");
        fp = fopen(out_name, "w");
        if (fp == NULL) {
            fprintf(stderr, "couldn't open '%s'\n", out_name);
            return false;
        }
        result = generate(fp);
        fclose(fp);
    }

    return result;
}

void usage()
{
    printf("usage: mcc [-d[istp]] filename\n");
    printf(" -di  debug ident\n");
    printf(" -ds  debug scanner\n");
    printf(" -dt  debug parser_trace\n");
    printf(" -dp  debug parser\n");
    printf(" -dn  debug node type\n");
    printf(" -dg  debug generate\n");
    exit(1);
}

int main(int argc ,char* argv[])
{
    int i, j;
    int result = 0;

    if (argc < 2)
        usage();
    init_symtab();
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'd') {
                for (j = 2; argv[i][j] != 0; j++) {
                    switch (argv[i][j]) {
                    case 'i': set_debug("ident"); break;
                    case 's': set_debug("scanner"); break;
                    case 't': set_debug("parser_trace"); break;
                    case 'p': set_debug("parser"); break;
                    case 'n': set_debug("node_type"); break;
                    case 'g': set_debug("gen"); break;
                    default: usage();
                    }
                }
            } else
                usage();
        } else if (!compile(argv[i]))
            result = 1;
    }
    term_symtab();
    return result;
}
