#include "minicc.h"

const char *source =
    "a 1\n"
    "aa 111\nb 22\n"
    "abc 'a' 123 \"abcdef\"\n"
    ", ~ ; : ? ( ) [ ] { } . ... = == ! != * *= / /=\n"
    "% %= ^ ^= & && &= | || |= + ++ += - -- -= ->\n"
    "< << <<= <= > >> >>= >= sizeof typedef extern static\n"
    "auto register char short int long signed unsigned\n"
    "float double const volatile void struct union\n"
    "enum if else switch case default while do for goto\n"
    "continue break return\n";

int scan_test(const char *source)
{
    SCANNER *scan;
    TOKEN tk;

    scan = open_scanner_text("test", source);
    if (scan == NULL)
        return 1;
    while  ((tk = next_token(scan)) != TK_EOF) {
        printf("%s(%d): %s\n", scan->pos.filename, scan->pos.line,
                scan_token_to_string(scan, tk));
    }
    close_scanner(scan);
    return 0;
}

int main(void)
{
    return scan_test(source);
}

