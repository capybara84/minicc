#include "minicc.h"

static SYMTAB *global_symtab = NULL;
static SYMTAB *current_symtab = NULL;
static SYMBOL *current_function = NULL;

SYMTAB *new_symtab(SYMTAB *up)
{
    SYMTAB *tab = (SYMTAB*) alloc(sizeof (SYMTAB));
    tab->head = tab->tail = NULL;
    tab->up = up;
    return tab;
}

bool init_symtab(void)
{
    global_symtab = new_symtab(NULL);
    current_symtab = global_symtab;
    return true;
}

void term_symtab(void)
{
    current_symtab = global_symtab = NULL;
}


SYMBOL *new_symbol(SYMBOL_KIND kind, STORAGE_CLASS sc, TYPE_QUALIFIER tq,
                    const char *id, TYPE *type)
{
    SYMBOL *p;

    assert(current_symtab);
    p = (SYMBOL*) alloc(sizeof (SYMBOL));
    p->next = NULL;
    if (current_symtab->tail == NULL) {
        assert(current_symtab->head == NULL);
        current_symtab->head = current_symtab->tail = p;
    } else {
        current_symtab->tail->next = p;
        current_symtab->tail = p;
    }
    p->sclass = sc;
    p->kind = kind;
    p->is_volatile = (tq == TQ_VOLATILE);
    p->id = id;
    p->type = type;
    p->tab = NULL;
    p->body = NULL;
    return p;
}

SYMBOL *lookup_symbol(const char *id)
{
    SYMTAB *tab;
    SYMBOL *sym;

    for (tab = current_symtab; tab != NULL; tab = tab->up) {
        for (sym = tab->head; sym != NULL; sym = sym->next) {
            if (sym->id == id)
                return sym;
        }
    }
    return NULL;
}


SYMTAB *enter_scope(void)
{
    SYMTAB *tab = new_symtab(current_symtab);
    return current_symtab = tab;
}

void leave_scope(void)
{
    assert(current_symtab->up);
    current_symtab = current_symtab->up;
}

void enter_function(SYMBOL *sym)
{
    current_function = sym;
    sym->tab = enter_scope();
}

void leave_function(void)
{
    leave_scope();
    current_function = NULL;
}

const char *get_sym_kind_string(SYMBOL_KIND kind)
{
    switch (kind) {
    case SK_LOCAL:      return "LOCAL";
    case SK_GLOBAL:     return "GLOBAL";
    case SK_PARAM:      return "PARAM";
    case SK_FUNC:       return "FUNC";
    }
    return NULL;
}

const char *get_sclass_string(STORAGE_CLASS sc)
{
    switch (sc) {
    case SC_DEFAULT:    return "DEFAULT";
    case SC_AUTO:       return "AUTO";
    case SC_REGISTER:   return "REGISTER";
    case SC_STATIC:     return "STATIC";
    case SC_EXTERN:     return "EXTERN";
    case SC_TYPEDEF:    return "TYPEDEF";
    }
    return NULL;
}

void fprint_symbol(FILE *fp, int indent, const SYMBOL *sym)
{
    fprintf(fp, "%*sSYM %s %s %s%s:", indent, "",
        sym->id, get_sym_kind_string(sym->kind),
                get_sclass_string(sym->sclass),
        (sym->is_volatile) ? " volatile" : "");
    fprint_type(fp, sym->type);
    fprintf(fp, "\n");
    if (sym->kind == SK_FUNC) {
        fprintf(fp, "%*s{\n", indent, "");
        fprint_symtab(fp, indent+2, sym->tab);
        fprint_node(fp, indent+2, sym->body);
        fprintf(fp, "%*s}\n", indent, "");
    }
}

void fprint_symtab(FILE *fp, int indent, const SYMTAB *tab)
{
    const SYMBOL *sym;
    if (tab == NULL)
        return;
    for (sym = tab->head; sym != NULL; sym = sym->next) {
        fprint_symbol(fp, indent, sym);
    }
}

void print_symtab(const SYMTAB *tab)
{
    fprint_symtab(stdout, 0, tab);
}

void print_global_symtab(void)
{
    printf("GLOBAL SYMTAB\n");
    print_symtab(global_symtab);
}
