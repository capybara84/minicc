#include "minicc.h"

TYPE *new_type(TYPE_KIND kind)
{
    TYPE *tp = (TYPE*) alloc(sizeof (TYPE));
    tp->kind = kind;
    tp->is_const = false;
    tp->type = NULL;
    tp->param = NULL;
    return tp;
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
    case T_STRUCT:  return "struct"; /*TODO*/
    case T_UNION:   return "union"; /*TODO*/
    case T_ENUM:    return "enum";  /*TODO*/
    case T_TYPEDEF_NAME:    return "typedef_name";  /*TODO*/
    case T_POINTER: return "pointer";   /*TODO*/
    case T_FUNC:    return "func";  /*TODO*/
    case T_SIGNED:  return "signed";    /*TODO*/
    case T_UNSIGNED:    return "unsigned";  /*TODO*/
    }
    return "?";
}
