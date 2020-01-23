#include "minicc.h"

PARAM *new_param(char *id, TYPE *typ)
{
    PARAM *param = (PARAM*) alloc(sizeof (PARAM));
    param->next = NULL;
    param->id = id;
    param->type = typ;
    param->is_register = false;
    return param;
}

void add_param(PARAM *top, PARAM *param)
{
    PARAM *p;
    assert(top);
    for (p = top; p->next != NULL; p = p->next)
        ;
    p->next = param;
}

TYPE *new_type(TYPE_KIND kind, TYPE *typ)
{
    TYPE *tp = (TYPE*) alloc(sizeof (TYPE));
    tp->kind = kind;
    tp->is_const = false;
    tp->type = typ;
    tp->param = NULL;
    tp->tag = NULL;
    return tp;
}

TYPE *dup_type(TYPE *typ)
{
    TYPE *tp = (TYPE*) alloc(sizeof (TYPE));
    tp->kind = typ->kind;
    tp->is_const = typ->is_const;
    tp->type = typ->type;
    tp->param = typ->param;
    tp->tag = typ->tag;
    return tp;
}

bool equal_type(const TYPE *tl, const TYPE *tr)
{
    PARAM *pl;
    PARAM *pr;

    if (tl == NULL && tr == NULL)
        return true;
    if (tl == NULL || tr == NULL)
        return false;
    if (tl->kind != tr->kind)
        return false;
    if (tl->is_const != tr->is_const)
        return false;
    if (tl->tag != tr->tag)
        return false;
    if (!equal_type(tl->type, tr->type))
        return false;
    pl = tl->param;
    pr = tr->param;
    while (pl != NULL && pr != NULL) {
        if (!equal_type(pl->type, pr->type))
            return false;
        if (pl->is_register != pr->is_register)
            return false;
        pl = pl->next;
        pr = pr->next;
    }
    if (pl != NULL || pr != NULL)
        return false;
    return true;
}


const char *get_type_string(const TYPE *typ)
{
    if (typ == NULL)
        return "NUL";
    switch (typ->kind) {
    case T_UNKNOWN: return "UNKNOWN";
    case T_VOID:    return "void";
    case T_NULL:    return "null";
    case T_CHAR:    return "char";
    case T_UCHAR:   return "uchar";
    case T_SHORT:   return "short";
    case T_USHORT:  return "ushort";
    case T_INT:     return "int";
    case T_UINT:    return "uint";
    case T_LONG:    return "long";
    case T_ULONG:   return "ulong";
    case T_FLOAT:   return "float";
    case T_DOUBLE:  return "double";
    case T_STRUCT:      return "struct"; /*TODO*/
    case T_UNION:       return "union"; /*TODO*/
    case T_ENUM:        return "enum";  /*TODO*/
    case T_TYPEDEF_NAME:    return "typedef_name";  /*TODO*/
    case T_POINTER:     return "pointer";   /*TODO*/
    case T_ARRAY:       return "array";   /*TODO*/
    case T_FUNC:        return "func";  /*TODO*/
    case T_SIGNED:      return "SIGNED";
    case T_UNSIGNED:    return "UNSIGNED";
    }
    return "?";
}

void fprint_param(FILE *fp, const PARAM *p)
{
    if (p->is_register)
        fprintf(fp, "register ");
    fprint_type(fp, p->type);
    if (p->id)
        fprintf(fp, " %s", p->id);
}

void fprint_type(FILE *fp, const TYPE *typ)
{
    const PARAM *p;
    if (typ == NULL)
        fprintf(fp, "NULL");
    else {
        if (typ->is_const)
            fprintf(fp, "const ");
        switch (typ->kind) {
        case T_UNKNOWN:
            fprintf(fp, "UNKNOWN");
            break;
        case T_VOID:
            fprintf(fp, "void");
            break;
        case T_NULL:
            fprintf(fp, "null");
            break;
        case T_CHAR:
            fprintf(fp, "char");
            break;
        case T_UCHAR:
            fprintf(fp, "uchar");
            break;
        case T_SHORT:
            fprintf(fp, "short");
            break;
        case T_USHORT:
            fprintf(fp, "ushort");
            break;
        case T_INT:
            fprintf(fp, "int");
            break;
        case T_UINT:
            fprintf(fp, "uint");
            break;
        case T_LONG:
            fprintf(fp, "long");
            break;
        case T_ULONG:
            fprintf(fp, "ulong");
            break;
        case T_FLOAT:
            fprintf(fp, "float");
            break;
        case T_DOUBLE:
            fprintf(fp, "double");
            break;
        case T_STRUCT:
            if (typ->tag)
                fprintf(fp, "struct %s", typ->tag);
            else
                fprintf(fp, "struct");
            break;
        case T_UNION:
            if (typ->tag)
                fprintf(fp, "union %s", typ->tag);
            else
                fprintf(fp, "union");
            break;
        case T_ENUM:
            if (typ->tag)
                fprintf(fp, "enum %s", typ->tag);
            else
                fprintf(fp, "enum");
            break;
        case T_TYPEDEF_NAME:
            fprintf(fp, "%s", typ->tag);
            break;
        case T_POINTER:
            fprintf(fp, "POINTER to ");
            fprint_type(fp, typ->type);
            break;
        case T_ARRAY:
            fprintf(fp, "ARRAY of ");
            fprint_type(fp, typ->type);
            break;
        case T_FUNC:
            fprintf(fp, "FUNC ");
            fprint_type(fp, typ->type);
            fprintf(fp, " (");
            for (p = typ->param; p != NULL; p = p->next) {
                fprint_param(fp, p);
                if (p->next)
                    fprintf(fp, ", ");
            }
            fprintf(fp, ")");
            break;
        case T_SIGNED:
            fprintf(fp, "SIGNED");
            break;
        case T_UNSIGNED:
            fprintf(fp, "UNSIGNED");
            break;
        }
    }
}

void print_type(const TYPE *typ)
{
    fprint_type(stdout, typ);
}
