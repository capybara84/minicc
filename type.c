#include "minicc.h"

const char *get_type_string(const TYPE *typ)
{
    if (typ == NULL)
        return "NUL";
    switch (typ->kind) {
    case T_UNKNOWN: return "UNKNOWN";
    case T_VOID:    return "void";
    case T_NULL:    return "null";
    }
    return "?";
}
