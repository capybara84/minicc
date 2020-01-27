#include "minicc.h"

#ifdef NDEBUG
# define ERR_OUT stderr
#else
# define ERR_OUT stdout
#endif

static int s_n_error;
static int s_n_warning;

#ifndef NDEBUG
struct debug {
    struct debug *next;
    const char *name;
};

static struct debug *s_debug = NULL;

bool is_debug(const char *s)
{
    struct debug *d;
    for (d = s_debug; d != NULL; d = d->next)
        if (strcmp(d->name, s) == 0)
            return true;
    return false;
}

void set_debug(const char *s)
{
    if (!is_debug(s)) {
        struct debug *d = (struct debug*) alloc(sizeof (struct debug));
        d->name = s;
        d->next = s_debug;
        s_debug = d;
    }
}
#endif


int get_num_errors(void)
{
    return s_n_error;
}

int get_num_warning(void)
{
    return s_n_warning;
}

void vwarning(const POS *pos, const char *s, va_list ap)
{
    fprintf(ERR_OUT, "%s(%d): warning: ", pos->filename, pos->line);
    vfprintf(ERR_OUT, s, ap);
    fprintf(ERR_OUT, "\n");
    s_n_warning++;
}

void warning(const POS *pos, const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    vwarning(pos, s, ap);
    va_end(ap);
}

void verror(const POS *pos, const char *s, va_list ap)
{
    fprintf(ERR_OUT, "%s(%d): error: ", pos->filename, pos->line);
    vfprintf(ERR_OUT, s, ap);
    fprintf(ERR_OUT, "\n");
    s_n_error++;
}

void error(const POS *pos, const char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    verror(pos, s, ap);
    va_end(ap);
}

void *alloc(size_t size)
{
    void *p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "out of memory\n");
        abort();
    }
    return p;
}

char *str_dup(const char *s)
{
    int len = strlen(s);
    char *d = (char*) alloc(len+1);
    strcpy(d, s);
    return d;
}

