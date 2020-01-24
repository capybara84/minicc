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
    }
}

void print_node(const NODE *np)
{
    fprint_node(stdout, 0, np);
}

