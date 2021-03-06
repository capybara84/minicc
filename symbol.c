#include "minicc.h"

static SYMTAB *global_symtab = NULL;
static SYMTAB *current_symtab = NULL;
static SYMBOL *current_function = NULL;

SYMTAB *get_global_symtab(void)
{
    return global_symtab;
}

SYMTAB *new_symtab(SYMTAB *up)
{
    SYMTAB *tab = (SYMTAB*) alloc(sizeof (SYMTAB));
    tab->head = tab->tail = NULL;
    tab->up = up;
    tab->scope = up ? up->scope+1 : 0;
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


SYMBOL *new_symbol(SYMBOL_KIND kind, const char *id, TYPE *type, int scope)
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
    p->kind = kind;
    p->id = id;
    p->type = type;
    p->scope = scope;
    p->num = 0;
    p->offset = 0;
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

bool sym_is_left_value(const SYMBOL *sym)
{
    assert(sym);
    if (sym->kind == SK_FUNC)
        return false;
    if (is_const_type(sym->type))
        return false;
    return true;
}

bool sym_is_static(const SYMBOL *sym)
{
    assert(sym);
    assert(sym->type);
    return sym->type->sclass == SC_STATIC;
}

bool sym_is_extern(const SYMBOL *sym)
{
    assert(sym);
    assert(sym->type);
    return sym->type->sclass == SC_EXTERN;
}

int get_current_func_local_offset(void)
{
    int offset;
    assert(current_function);
    offset = current_function->offset;
    current_function->offset += BYTE_INT;
    return offset;
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

void fprint_var_comment(FILE *fp, const SYMBOL *sym)
{
    assert(sym);
    fprintf(fp, "# %s %s (num %d, offset %d): ",
            get_sym_kind_string(sym->kind), sym->id, sym->num, sym->offset);
    fprint_type(fp, sym->type);
    fprintf(fp, "\n");
}

void fprint_func_comment(FILE *fp, const SYMBOL *sym)
{
    const SYMBOL *sp;
    assert(sym);
    assert(sym->kind == SK_FUNC);
    fprintf(fp, "# FUNC %s (param %d, local %d): ", sym->id, sym->num, sym->offset);
    fprint_type(fp, sym->type);
    fprintf(fp, "\n");
    if (sym->tab) {
        for (sp = sym->tab->head; sp != NULL; sp = sp->next)
            fprint_var_comment(fp, sp);
    }
}

void fprint_symbol(FILE *fp, int indent, const SYMBOL *sym)
{
    fprintf(fp, "%*sSYM %s %s (p%d, l%d):", indent, "",
            sym->id, get_sym_kind_string(sym->kind), sym->num, sym->offset);
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
    fprintf(fp, "scope:%d\n", tab->scope);
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
