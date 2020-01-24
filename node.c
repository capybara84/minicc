#include "minicc.h"

NODE *new_node(NODE_KIND kind, const POS *pos)
{
    NODE *np = (NODE*) alloc(sizeof (NODE));
    np->kind = kind;
    np->pos = *pos;
    np->u.link.left = NULL;
    np->u.link.right = NULL;
    return np;
}

NODE *new_node1(NODE_KIND kind, const POS *pos, NODE *np)
{
    NODE *node = new_node(kind, pos);
    node->u.link.left = np;
    return node;
}

NODE *new_node2(NODE_KIND kind, const POS *pos, NODE *left, NODE *right)
{
    NODE *np = new_node(kind, pos);
    np->u.link.left = left;
    np->u.link.right = right;
    return np;
}

NODE *new_node_num(NODE_KIND kind, const POS *pos, int num)
{
    NODE *np = new_node(kind, pos);
    np->u.num = num;
    return np;
}

NODE *new_node_id(NODE_KIND kind, const POS *pos, const char *id)
{
    NODE *np = new_node(kind, pos);
    np->u.id = id;
    return np;
}

NODE *new_node_idnode(NODE_KIND kind, const POS *pos, NODE *ep, const char *id)
{
    NODE *np = new_node(kind, pos);
    np->u.idnode.node = ep;
    np->u.idnode.id = id;
    return np;
}

NODE *new_node_case(NODE_KIND kind, const POS *pos, int num, NODE *body)
{
    NODE *np = new_node(kind, pos);
    np->u.ncase.num = num;
    np->u.ncase.body = body;
    return np;
}

NODE *node_link(NODE_KIND kind, const POS *pos, NODE *n, NODE *top)
{
    NODE *np = new_node2(kind, pos, n, NULL);
    NODE *p;
    if (top == NULL)
        return np;
    for (p = top; p->u.link.right != NULL; p = p->u.link.right)
        ;
    p->u.link.right = np;
    return top;
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
    case NK_THEN:
    case NK_SWITCH:
    case NK_CASE:
    case NK_DEFAULT:
    case NK_WHILE:
    case NK_DO:
    case NK_FOR:
    case NK_FOR2:
    case NK_FOR3:
    case NK_GOTO:
    case NK_CONTINUE:
    case NK_BREAK:
        /*TODO*/
        break;
    case NK_RETURN:
    case NK_EXPR:
    case NK_EXPR_LINK:
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
    case NK_EQ:
    case NK_NEQ:
    case NK_LT:
    case NK_GT:
    case NK_LE:
    case NK_GE:
    case NK_SHL:
    case NK_SHR:
    case NK_ADD:
    case NK_SUB:
    case NK_MUL:
    case NK_DIV:
    case NK_MOD:
    case NK_CAST:
    case NK_PREINC:
    case NK_PREDEC:
    case NK_SIZEOF:
    case NK_COND:
    case NK_COND2:
    case NK_LOR:
    case NK_LAND:
    case NK_OR:
    case NK_XOR:
    case NK_AND:
    case NK_ARRAY:
    case NK_CALL:
    case NK_DOT:
    case NK_PTR:
    case NK_POSTINC:
    case NK_POSTDEC:
    case NK_ID:
    case NK_CHAR_LIT:
    case NK_INT_LIT:
    case NK_UINT_LIT:
    case NK_LONG_LIT:
    case NK_ULONG_LIT:
    case NK_FLOAT_LIT:
    case NK_DOUBLE_LIT:
    case NK_STRING_LIT:
    case NK_ARG:
    case NK_ADDR:
    case NK_DEREF:
    case NK_UPLUS:
    case NK_UMINUS:
    case NK_COMPLEMENT:
    case NK_NOT:
    default:    /*TODO*/
        /*TODO*/
        break;
    }
}

void print_node(const NODE *np)
{
    fprint_node(stdout, 0, np);
}

