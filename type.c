#include "minicc.h"

TYPE g_type_uchar = { T_UCHAR, SC_DEFAULT, TQ_DEFAULT, NULL, NULL, NULL };
TYPE g_type_int = { T_INT, SC_DEFAULT, TQ_DEFAULT, NULL, NULL, NULL };

PARAM *new_param(char *id, TYPE *typ)
{
    PARAM *param = (PARAM*) alloc(sizeof (PARAM));
    param->next = NULL;
    param->id = id;
    param->type = typ;
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
    tp->sclass = SC_DEFAULT;
    tp->tqual = TQ_DEFAULT;
    tp->type = typ;
    tp->param = NULL;
    tp->tag = NULL;
    return tp;
}

TYPE *dup_type(TYPE *typ)
{
    TYPE *tp = (TYPE*) alloc(sizeof (TYPE));
    tp->kind = typ->kind;
    tp->sclass = typ->sclass;
    tp->tqual = typ->tqual;
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
    if (tl->sclass != tr->sclass)
        return false;
    if (tl->tqual != tr->tqual)
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
        pl = pl->next;
        pr = pr->next;
    }
    if (pl != NULL || pr != NULL)
        return false;
    return true;
}

TYPE *type_check_array(const POS *pos, const TYPE *arr, const TYPE *e)
{
    /*TODO impl */
    return &g_type_int;
}

TYPE *type_check_call(const POS *pos, const TYPE *fn, const TYPE *arg)
{
    /*TODO impl */
    return &g_type_int;
}

TYPE *type_check_idnode(const POS *pos, NODE_KIND kind,
                        const TYPE *e, const char *id)
{
    /*TODO impl */
    return &g_type_int;
}

TYPE *type_check_postfix(const POS *pos, NODE_KIND kind, TYPE *e)
{
    /*TODO impl */
    return e;
}

TYPE *type_check_unary(const POS *pos, NODE_KIND kind, TYPE *e)
{
    /*TODO impl */
    return e;
}


TYPE *type_check_bin(const POS *pos, NODE_KIND kind,
                        TYPE *lhs, TYPE *rhs)
{
    switch (kind) {
    case NK_ASSIGN:
    case NK_AS_MUL:
    case NK_AS_DIV:
    case NK_AS_MOD:
    case NK_AS_ADD:
    case NK_AS_SUB:
    case NK_AS_SHL:
    case NK_AS_SHR:
    case NK_AS_AND:
    case NK_AS_XOR:
    case NK_AS_OR:
        /*TODO impl */
        return lhs;
    case NK_EQ:
    case NK_NEQ:
        /*TODO impl */
        return &g_type_int;
    case NK_LT:
    case NK_GT:
    case NK_LE:
    case NK_GE:
        /*TODO impl */
        return &g_type_int;
    case NK_SHL:
    case NK_SHR:
    case NK_ADD:
    case NK_SUB:
    case NK_MUL:
    case NK_DIV:
    case NK_MOD:
    case NK_AND:
    case NK_XOR:
    case NK_OR:
        /*TODO impl */
        return &g_type_int;
    case NK_LAND:
        /*TODO impl */
        return &g_type_int;
    case NK_LOR:
        /*TODO impl */
        return &g_type_int;
    case NK_COND2:
        /*TODO impl */
        return lhs;
    case NK_SIZEOF:
        /*TODO impl */
        return &g_type_int;
    default:
        assert(0);
        break;
    }
    error(pos, "'%s' type mismatch", get_node_op_string(kind));
    return &g_type_int;
}

void type_check_value(const POS *pos, const TYPE *t)
{
    /*TODO impl */
}

const char *get_type_string(const TYPE *typ)
{
    if (typ == NULL)
        return "NUL";
    switch (typ->kind) {
    case T_UNKNOWN:     return "UNKNOWN";
    case T_VOID:        return "void";
    case T_NULL:        return "null";
    case T_CHAR:        return "char";
    case T_UCHAR:       return "uchar";
    case T_SHORT:       return "short";
    case T_USHORT:      return "ushort";
    case T_INT:         return "int";
    case T_UINT:        return "uint";
    case T_LONG:        return "long";
    case T_ULONG:       return "ulong";
    case T_FLOAT:       return "float";
    case T_DOUBLE:      return "double";
    case T_STRUCT:      return "struct"; /*TODO*/
    case T_UNION:       return "union"; /*TODO*/
    case T_ENUM:        return "enum";  /*TODO*/
    case T_TYPEDEF_NAME:return "typedef_name";  /*TODO*/
    case T_POINTER:     return "pointer";   /*TODO*/
    case T_ARRAY:       return "array";   /*TODO*/
    case T_FUNC:        return "func";  /*TODO*/
    case T_SIGNED:      return "SIGNED";
    case T_UNSIGNED:    return "UNSIGNED";
    }
    return "?";
}

const char *get_sclass_string(STORAGE_CLASS sc)
{
    switch (sc) {
    case SC_DEFAULT:    return "SC_DEFAULT";
    case SC_AUTO:       return "auto";
    case SC_REGISTER:   return "register";
    case SC_STATIC:     return "static";
    case SC_EXTERN:     return "extern";
    case SC_TYPEDEF:    return "typedef";
    }
    return "?";
}

void fprint_param(FILE *fp, const PARAM *p)
{
    if (p->type == NULL)
        fprintf(fp, " ...");
    else {
        fprint_type(fp, p->type);
        if (p->id)
            fprintf(fp, " %s", p->id);
    }
}

void fprint_type(FILE *fp, const TYPE *typ)
{
    const PARAM *p;
    if (typ == NULL)
        fprintf(fp, "NULL");
    else {
        if (typ->sclass != SC_DEFAULT)
            fprintf(fp, "%s ", get_sclass_string(typ->sclass));
        if (typ->tqual != TQ_DEFAULT) {
            if (typ->tqual & TQ_CONST)
                fprintf(fp, "const ");
            if (typ->tqual & TQ_VOLATILE)
                fprintf(fp, "volatile ");
        }
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
            fprintf(fp, "ARRAY [] of ");
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
