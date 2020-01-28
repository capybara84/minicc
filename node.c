#include "minicc.h"

NODE *new_node(NODE_KIND kind, const POS *pos, TYPE *typ)
{
    NODE *np = (NODE*) alloc(sizeof (NODE));
    np->kind = kind;
    np->pos = *pos;
    np->type = typ;
    np->u.link.left = NULL;
    np->u.link.right = NULL;
    return np;
}

NODE *new_node1(NODE_KIND kind, const POS *pos, TYPE *typ, NODE *np)
{
    NODE *node = new_node(kind, pos, typ);
    node->u.link.left = np;
    return node;
}

NODE *new_node2(NODE_KIND kind, const POS *pos, TYPE *typ, NODE *left, NODE *right)
{
    NODE *np = new_node(kind, pos, typ);
    np->u.link.left = left;
    np->u.link.right = right;
    return np;
}

NODE *new_node_num(NODE_KIND kind, const POS *pos, TYPE *typ, int num)
{
    NODE *np = new_node(kind, pos, typ);
    np->u.num = num;
    return np;
}

NODE *new_node_id(NODE_KIND kind, const POS *pos, TYPE *typ, const char *id)
{
    NODE *np = new_node(kind, pos, typ);
    np->u.id = id;
    return np;
}

NODE *new_node_sym(NODE_KIND kind, const POS *pos, TYPE *typ, SYMBOL *sym)
{
    NODE *np = new_node(kind, pos, typ);
    np->u.sym = sym;
    return np;
}

NODE *new_node_idnode(NODE_KIND kind, const POS *pos, TYPE *typ,
                        NODE *ep, const char *id)
{
    NODE *np = new_node(kind, pos, typ);
    np->u.idnode.node = ep;
    np->u.idnode.id = id;
    return np;
}

NODE *new_node_num_node(NODE_KIND kind, const POS *pos, TYPE *typ,
                    NODE *nodep, int num)
{
    NODE *np = new_node(kind, pos, typ);
    np->u.num_node.node = nodep;
    np->u.num_node.num = num;
    return np;
}

NODE *node_link(NODE_KIND kind, const POS *pos, NODE *n, NODE *top)
{
    NODE *np = new_node2(kind, pos, NULL, n, NULL);
    NODE *p;
    if (top == NULL)
        return np;
    for (p = top; p->u.link.right != NULL; p = p->u.link.right)
        ;
    p->u.link.right = np;
    return top;
}

bool calc_constant_expr(const NODE *np, int *result)
{
    int lval, rval;
    assert(np);
    assert(result);

    switch (np->kind) {
    case NK_EQ:
    case NK_NEQ:
    case NK_LT:
    case NK_GT:
    case NK_LE:
    case NK_GE:
        /*TODO*/
        return false;
    case NK_SHL:
    case NK_SHR:
        /*TODO*/
        return false;
    case NK_ADD:
    case NK_SUB:
    case NK_MUL:
    case NK_DIV:
    case NK_MOD:
        if (!calc_constant_expr(np->u.link.left, &lval))
            return false;
        if (!calc_constant_expr(np->u.link.right, &rval))
            return false;
        switch (np->kind) {
        case NK_ADD:
            *result = lval + rval;
            break;
        case NK_SUB:
            *result = lval - rval;
            break;
        case NK_MUL:
            *result = lval * rval;
            break;
        case NK_DIV:
            if (rval == 0) {
                error(&np->pos, "divide by zero");
                return false;
            }
            *result = lval / rval;
            break;
        case NK_MOD:
            if (rval == 0) {
                error(&np->pos, "divide by zero");
                return false;
            }
            *result = lval % rval;
            break;
        default: assert(0);
        }
        return true;
    case NK_LOR:
    case NK_LAND:
        /*TODO*/
        return false;
    case NK_OR:
    case NK_XOR:
    case NK_AND:
        /*TODO*/
        return false;
    case NK_UPLUS:
    case NK_UMINUS:
    case NK_COMPLEMENT:
    case NK_NOT:
        /*TODO*/
        return false;
    case NK_CAST:
        /*TODO*/
        return false;
    case NK_COND:
        /*TODO*/
        return false;
    case NK_ID:
        /*TODO*/
        return false;
    case NK_CHAR_LIT:
    case NK_INT_LIT:
        *result = np->u.num;
        return true;
    case NK_UINT_LIT:
    case NK_LONG_LIT:
    case NK_ULONG_LIT:
        /*TODO*/
        return false;
    default:
        error(&np->pos, "expect integer constant expression");
        break;
    }
    return false;
}

const char *get_node_op_string(NODE_KIND kind)
{
    switch (kind) {
    case NK_ASSIGN:     return "=";
    case NK_AS_MUL:     return "*=";
    case NK_AS_DIV:     return "/=";
    case NK_AS_MOD:     return "%=";
    case NK_AS_ADD:     return "+=";
    case NK_AS_SUB:     return "-=";
    case NK_AS_SHL:     return "<<=";
    case NK_AS_SHR:     return ">>=";
    case NK_AS_AND:     return "&=";
    case NK_AS_XOR:     return "^=";
    case NK_AS_OR:      return "|=";
    case NK_EQ:         return "==";
    case NK_NEQ:        return "!=";
    case NK_LT:         return "<";
    case NK_GT:         return ">";
    case NK_LE:         return "<=";
    case NK_GE:         return ">=";
    case NK_SHL:        return "<<";
    case NK_SHR:        return ">>";
    case NK_ADD:
    case NK_UPLUS:      return "+";
    case NK_SUB:
    case NK_UMINUS:     return "-";
    case NK_MUL:
    case NK_DEREF:      return "*";
    case NK_DIV:        return "/";
    case NK_MOD:        return "%";
    case NK_LOR:        return "||";
    case NK_LAND:       return "&&";
    case NK_OR:         return "|";
    case NK_XOR:        return "^";
    case NK_AND:
    case NK_ADDR:       return "&";
    case NK_COMPLEMENT: return "~";
    case NK_NOT:        return "!";
    case NK_POSTINC:
    case NK_PREINC:     return "++";
    case NK_POSTDEC:
    case NK_PREDEC:     return "--";
    case NK_SIZEOF:     return "sizeof ";
    default: assert(0);
    }
    return NULL;
}

void fprint_node(FILE *fp, int indent, const NODE *np)
{
    if (np == NULL)
        return;
    switch (np->kind) {
    case NK_COMPOUND:
        if (is_debug("node")) {
            fprintf(fp, "%*s%s(%d):", indent, "",
                            np->pos.filename, np->pos.line);
        }
        fprintf(fp, "%*s{\n", indent, "");
        if (np->u.comp.symtab) {
            fprint_symtab(fp, indent+2, np->u.comp.symtab);
        }
        fprint_node(fp, indent+2, np->u.comp.node);
        fprintf(fp, "%*s}\n", indent, "");
        break;
    case NK_LINK:
        fprint_node(fp, indent, np->u.link.left);
        fprint_node(fp, indent, np->u.link.right);
        break;
    case NK_IF:
        fprintf(fp, "%*sif (", indent, "");
        fprint_node(fp, indent, np->u.link.left);
        fprintf(fp, ")\n");
        assert(np->u.link.right);
        assert(np->u.link.right->kind == NK_THEN);
        fprint_node(fp, indent+2, np->u.link.right->u.link.left);
        if (np->u.link.right->u.link.right) {
            fprintf(fp, "%*selse\n", indent, "");
            fprint_node(fp, indent+2, np->u.link.right->u.link.right);
        }
        break;
    case NK_THEN:
        assert(0);
        break;
    case NK_SWITCH:
        fprintf(fp, "%*sswitch (", indent, "");
        fprint_node(fp, indent, np->u.link.left);
        fprintf(fp, ")\n");
        fprint_node(fp, indent, np->u.link.right);
        break;
    case NK_CASE:
        fprintf(fp, "%*scase %d:", indent, "", np->u.num_node.num);
        fprintf(fp, ":\n");
        fprint_node(fp, indent+2, np->u.num_node.node);
        break;
    case NK_DEFAULT:
        fprintf(fp, "%*sdefault:\n", indent, "");
        fprint_node(fp, indent+2, np->u.link.left);
        break;
    case NK_WHILE:
        fprintf(fp, "%*swhile (", indent, "");
        fprint_node(fp, indent, np->u.link.left);
        fprintf(fp, ")\n");
        fprint_node(fp, indent+2, np->u.link.right);
        break;
    case NK_DO:
        fprintf(fp, "%*sdo\n", indent, "");
        fprint_node(fp, indent+2, np->u.link.left);
        fprintf(fp, "%*swhile (", indent, "");
        fprint_node(fp, indent, np->u.link.right);
        fprintf(fp, ");\n");
        break;
    case NK_FOR:
        {
            NODE *f2, *f3;
            f2 = np->u.link.right;
            assert(f2 && f2->kind == NK_FOR2);
            f3 = f2->u.link.right;
            assert(f3 && f3->kind == NK_FOR3);
            fprintf(fp, "%*sfor (", indent, "");
            fprint_node(fp, indent, np->u.link.left);
            fprintf(fp, "; ");
            fprint_node(fp, indent, f2->u.link.left);
            fprintf(fp, "; ");
            fprint_node(fp, indent, f3->u.link.left);
            fprintf(fp, ")\n");
            fprint_node(fp, indent+2, f3->u.link.right);
        }
        break;
    case NK_FOR2:
    case NK_FOR3:
        assert(0);
        break;
    case NK_GOTO:
        fprintf(fp, "%*sgoto %s;\n", indent, "", np->u.id);
        break;
    case NK_CONTINUE:
        fprintf(fp, "%*scontinue;\n", indent, "");
        break;
    case NK_BREAK:
        fprintf(fp, "%*sbreak;\n", indent, "");
        break;
    case NK_RETURN:
        fprintf(fp, "%*sreturn ", indent, "");
        fprint_node(fp, indent, np->u.link.left);
        fprintf(fp, ";\n");
        break;
    case NK_LABEL:
        fprintf(fp, "%*s%s:\n", indent-2, "", np->u.idnode.id);
        fprint_node(fp, indent, np->u.idnode.node);
        break;
    case NK_EXPR:
        fprintf(fp, "%*s", indent, "");
        fprint_node(fp, indent, np->u.link.left);
        fprintf(fp, ";\n");
        break;
    case NK_EXPR_LINK:
        fprint_node(fp, indent, np->u.link.left);
        fprintf(fp, ", ");
        fprint_node(fp, indent, np->u.link.right);
        break;
    case NK_ASSIGN: case NK_AS_MUL: case NK_AS_DIV: case NK_AS_MOD:
    case NK_AS_ADD: case NK_AS_SUB: case NK_AS_SHL: case NK_AS_SHR:
    case NK_AS_AND: case NK_AS_XOR: case NK_AS_OR: case NK_EQ:
    case NK_NEQ: case NK_LT: case NK_GT: case NK_LE: case NK_GE:
    case NK_SHL: case NK_SHR: case NK_ADD: case NK_SUB: case NK_MUL:
    case NK_DIV: case NK_MOD: case NK_LOR: case NK_LAND: case NK_OR:
    case NK_XOR: case NK_AND:
        fprintf(fp, "(");
        fprint_node(fp, indent, np->u.link.left);
        fprintf(fp, " %s ", get_node_op_string(np->kind));
        fprint_node(fp, indent, np->u.link.right);
        fprintf(fp, ")");
        if (is_debug("node_type")) {
            fprintf(fp, ":");
            fprint_type(fp, np->type);
        }
        break;
    case NK_ADDR: case NK_DEREF: case NK_UPLUS: case NK_UMINUS:
    case NK_COMPLEMENT: case NK_NOT: case NK_PREINC:
    case NK_PREDEC: case NK_SIZEOF:
        fprintf(fp, "%s", get_node_op_string(np->kind));
        fprint_node(fp, indent, np->u.link.left);
        if (is_debug("node_type")) {
            fprintf(fp, ":");
            fprint_type(fp, np->type);
        }
        break;
    case NK_POSTINC:
    case NK_POSTDEC:
        fprint_node(fp, indent, np->u.link.left);
        fprintf(fp, "%s", get_node_op_string(np->kind));
        if (is_debug("node_type")) {
            fprintf(fp, ":");
            fprint_type(fp, np->type);
        }
        break;
    case NK_CAST:
        /*TODO impl cast */
        break;
    case NK_COND:
        {
            NODE *e, *e2, *e3;
            e = np->u.link.right;
            assert(e && e->kind == NK_COND2);
            e2 = e->u.link.left;
            e3 = e->u.link.right;
            fprintf(fp, "(");
            fprint_node(fp, indent, np->u.link.left);
            fprintf(fp, " ? ");
            fprint_node(fp, indent, e2);
            fprintf(fp, " : ");
            fprint_node(fp, indent, e3);
            fprintf(fp, ")");
        }
        if (is_debug("node_type")) {
            fprintf(fp, ":");
            fprint_type(fp, np->type);
        }
        break;
    case NK_COND2:
        assert(0);
        break;
    case NK_ARRAY:
        fprint_node(fp, indent, np->u.link.left);
        fprintf(fp, "[");
        fprint_node(fp, indent, np->u.link.right);
        fprintf(fp, "]");
        if (is_debug("node_type")) {
            fprintf(fp, ":");
            fprint_type(fp, np->type);
        }
        break;
    case NK_CALL:
        fprint_node(fp, indent, np->u.link.left);
        fprintf(fp, "(");
        fprint_node(fp, indent, np->u.link.right);
        fprintf(fp, ")");
        if (is_debug("node_type")) {
            fprintf(fp, ":");
            fprint_type(fp, np->type);
        }
        break;
    case NK_ARG:
        fprint_node(fp, indent, np->u.link.left);
        if (np->u.link.right) {
            fprintf(fp, ", ");
            fprint_node(fp, indent, np->u.link.right);
        }
        break;
    case NK_DOT:
        fprint_node(fp, indent, np->u.idnode.node);
        fprintf(fp, ".%s", np->u.idnode.id);
        if (is_debug("node_type")) {
            fprintf(fp, ":");
            fprint_type(fp, np->type);
        }
        break;
    case NK_PTR:
        fprint_node(fp, indent, np->u.idnode.node);
        fprintf(fp, "->%s", np->u.idnode.id);
        if (is_debug("node_type")) {
            fprintf(fp, ":");
            fprint_type(fp, np->type);
        }
        break;
    case NK_ID:
        if (np->u.sym == NULL)
            fprintf(fp, "<NULL>");
        else
            fprintf(fp, "%s", np->u.sym->id);
        if (is_debug("node_type")) {
            fprintf(fp, ":");
            fprint_type(fp, np->type);
        }
        break;
    case NK_CHAR_LIT:
        fprintf(fp, "'%c'", np->u.num);
        if (is_debug("node_type")) {
            fprintf(fp, ":");
            fprint_type(fp, np->type);
        }
        break;
    case NK_INT_LIT:
        fprintf(fp, "%d", np->u.num); 
        if (is_debug("node_type")) {
            fprintf(fp, ":");
            fprint_type(fp, np->type);
        }
        break;
    case NK_UINT_LIT:
    case NK_LONG_LIT:
    case NK_ULONG_LIT:
    case NK_FLOAT_LIT:
    case NK_DOUBLE_LIT:
    case NK_STRING_LIT:
        /*TODO impl literal */
        break;
    }
}

void print_node(const NODE *np)
{
    fprint_node(stdout, 0, np);
}

