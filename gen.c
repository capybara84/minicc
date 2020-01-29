#include "minicc.h"

static int s_label_num = 0;
#define NUM_REG_PARAM 6
static const char *s_param_reg32[NUM_REG_PARAM] = {
    "edi", "esi", "edx", "ecx", "r8d", "r9d",
};
static int s_param_offset32[NUM_REG_PARAM] = {
    4, 8, 12, 16, 20, 24
};

static int new_label(void)
{
    return s_label_num++;
}

static const char *get_var_addr(const SYMBOL *sym)
{
    static char buf[64];
    switch (sym->kind) {
    case SK_GLOBAL:
        sprintf(buf, "%s", sym->id);
        break;
    case SK_LOCAL:
        sprintf(buf, "[rbp-%d]", sym->offset + 8);
        break;
    case SK_PARAM:
        if (sym->num < NUM_REG_PARAM)
            sprintf(buf, "[rbp-%d]", s_param_offset32[sym->num]);
        else
            sprintf(buf, "[rbp+%d]", sym->offset + 16 - NUM_REG_PARAM *4);
        break;
    default:
        assert(0);
        buf[0] = 0;
        break;
    }
    return buf;
}

static bool gen_assign_expr(FILE *fp, NODE *np)
{
    SYMBOL *sym;
    if (np == NULL)
        return false;
    switch (np->kind) {
    case NK_ID:
        assert(np->u.sym);
        sym = np->u.sym;
        if (sym->kind == SK_FUNC)
            break;
        fprintf(fp, "    mov %s,eax ; %s\n", get_var_addr(sym), sym->id);
        return true;
    case NK_DOT:
    case NK_PTR:
        /*TODO*/
    default:
        break;
    }
    error(&np->pos, "left value required");
    return false;
}

static bool gen_expr(FILE *fp, NODE *np)
{
    if (np == NULL)
        return true;
    switch (np->kind) {
    case NK_ASSIGN:
        if (!gen_expr(fp, np->u.link.right))
            return false;
        if (gen_assign_expr(fp, np->u.link.left))
            return false;
        break;
    case NK_AS_MUL: case NK_AS_DIV: case NK_AS_MOD:
    case NK_AS_ADD: case NK_AS_SUB: case NK_AS_SHL: case NK_AS_SHR:
    case NK_AS_AND: case NK_AS_XOR: case NK_AS_OR:
        break;
    case NK_EQ: case NK_NEQ: case NK_LT: case NK_GT: case NK_LE: case NK_GE:
    case NK_SHL: case NK_SHR: case NK_ADD: case NK_SUB: case NK_MUL:
    case NK_DIV: case NK_MOD: case NK_OR: case NK_XOR: case NK_AND:
        gen_expr(fp, np->u.link.right);
        fprintf(fp, "    push eax\n");
        gen_expr(fp, np->u.link.left);
        fprintf(fp, "    pop edi\n");
        switch (np->kind) {
        case NK_EQ:
            fprintf(fp, "    cmp eax, edi\n");
            fprintf(fp, "    sete al\n");
            fprintf(fp, "    movzb eax, al\n");
            break;
        case NK_NEQ:
            fprintf(fp, "    cmp eax, edi\n");
            fprintf(fp, "    setne al\n");
            fprintf(fp, "    movzb eax, al\n");
            break;
        case NK_LT:
        case NK_GT:
        case NK_LE:
        case NK_GE:
            /*TODO*/
            break;
        case NK_SHL:
        case NK_SHR:
            /*TODO*/
            break;
        case NK_ADD:
            /*TODO consider type (bits) */
            fprintf(fp, "    add eax, edi\n");
            break;
        case NK_SUB:
            fprintf(fp, "    sub eax, edi\n");
            break;
        case NK_MUL:
            fprintf(fp, "    imul edi\n");
            break;
        case NK_DIV:
            fprintf(fp, "    cdq\n");
            fprintf(fp, "    idiv edi\n");
            break;
        case NK_MOD:
        case NK_OR:
        case NK_XOR:
        case NK_AND:
            /*TODO*/
            break;
        default:
            break;
        }
        break;
    case NK_LOR: case NK_LAND:
    case NK_ADDR: case NK_DEREF: case NK_UPLUS: case NK_UMINUS:
    case NK_COMPLEMENT: case NK_NOT: case NK_PREINC:
    case NK_PREDEC: case NK_SIZEOF:
        break;
    case NK_POSTINC:
    case NK_POSTDEC:
        break;
    case NK_CAST:
    case NK_COND:
    case NK_COND2:
    case NK_ARRAY:
    case NK_CALL:
    case NK_ARG:
    case NK_DOT:
    case NK_PTR:
        break;
    case NK_ID:
        assert(np->u.sym);
        if (np->u.sym->kind == SK_FUNC) {
            fprintf(fp, ";FUNC %s\n", np->u.sym->id);
            /*TODO*/
        } else {
            fprintf(fp, "    mov eax,%s ; %s\n",
                        get_var_addr(np->u.sym), np->u.sym->id);
        }
        break;
    case NK_CHAR_LIT:
        fprintf(fp, "    mov al, %d\n", np->u.num);
        /*TODO*/
        break;
    case NK_INT_LIT:
        fprintf(fp, "    mov eax, %d\n", np->u.num);
        break;
    case NK_UINT_LIT:
    case NK_LONG_LIT:
    case NK_ULONG_LIT:
    case NK_FLOAT_LIT:
    case NK_DOUBLE_LIT:
    case NK_STRING_LIT:
        break;
    default:
        error(&np->pos, "couldn't generate expression code");
        return false;
    }
    return true;
}

static bool gen_stmt(FILE *fp, NODE *np)
{
    int l1, l2;

    if (np == NULL)
        return true;

    switch (np->kind) {
    case NK_COMPOUND:
        fprintf(fp, "; %s(%d)\n", np->pos.filename, np->pos.line);
        if (!gen_stmt(fp, np->u.comp.node))
            return false;
        break;
    case NK_LINK:
        if (!gen_stmt(fp, np->u.link.left))
            return false;
        if (!gen_stmt(fp, np->u.link.right))
            return false;
        break;
    case NK_IF:
        fprintf(fp, "; %s(%d) IF\n", np->pos.filename, np->pos.line);
        gen_expr(fp, np->u.link.left);
        fprintf(fp, "    cmp eax, 0\n");
        l1 = new_label();
        fprintf(fp, "    je .L%d\n", l1);
        assert(np->u.link.right);
        assert(np->u.link.right->kind == NK_THEN);
        gen_stmt(fp, np->u.link.right->u.link.left);
        l2 = new_label();
        fprintf(fp, "    jmp .L%d\n", l2);
        fprintf(fp, ".L%d:\n", l1);
        if (np->u.link.right->u.link.right) {
            gen_stmt(fp, np->u.link.right->u.link.right);
        }
        fprintf(fp, ".L%d:\n", l2);
        break;
    case NK_SWITCH:
        fprintf(fp, "; %s(%d) SWITCH\n", np->pos.filename, np->pos.line);
        /*TODO*/
        break;
    case NK_CASE:
        fprintf(fp, "; %s(%d) CASE\n", np->pos.filename, np->pos.line);
        /*TODO*/
        break;
    case NK_DEFAULT:
        fprintf(fp, "; %s(%d) DEFAULT\n", np->pos.filename, np->pos.line);
        /*TODO*/
        break;
    case NK_WHILE:
        fprintf(fp, "; %s(%d) WHILE\n", np->pos.filename, np->pos.line);
        l1 = new_label();
        fprintf(fp, ".L%d:\n", l1);
        gen_expr(fp, np->u.link.left);
        fprintf(fp, "    cmp, eax, 0\n");
        l2 = new_label();
        fprintf(fp, "    je .L%d\n", l2);
        gen_stmt(fp, np->u.link.right);
        fprintf(fp, "    jmp .L%d\n", l1);
        fprintf(fp, ".L%d:\n", l2);
        break;
    case NK_DO:
        fprintf(fp, "; %s(%d) DO\n", np->pos.filename, np->pos.line);
        /*TODO*/
        break;
    case NK_FOR:
        fprintf(fp, "; %s(%d) FOR\n", np->pos.filename, np->pos.line);
        /*TODO*/
        break;
    case NK_GOTO:
        fprintf(fp, "; %s(%d) GOTO\n", np->pos.filename, np->pos.line);
        /*TODO*/
        break;
    case NK_CONTINUE:
        fprintf(fp, "; %s(%d) CONTINUE\n", np->pos.filename, np->pos.line);
        /*TODO*/
        break;
    case NK_BREAK:
        fprintf(fp, "; %s(%d) BREAK\n", np->pos.filename, np->pos.line);
        /*TODO*/
        break;
    case NK_RETURN:
        fprintf(fp, "; %s(%d) RETURN\n", np->pos.filename, np->pos.line);
        if (np->u.link.left) {
            if (!gen_expr(fp, np->u.link.left))
                return false;
            fprintf(fp, "    mov rsp, rbp\n");
            fprintf(fp, "    pop rbp\n");
            fprintf(fp, "    ret\n");
        }
        break;
    case NK_LABEL:
        fprintf(fp, "; %s(%d) LABEL %s\n", np->pos.filename, np->pos.line,
                    np->u.idnode.id);
        break;
    case NK_EXPR:
        fprintf(fp, "; %s(%d) EXPR ", np->pos.filename, np->pos.line);
        fprint_node(fp, 0, np->u.link.left);
        fprintf(fp, "\n");
        if (!gen_expr(fp, np->u.link.left))
            return false;
        break;
    case NK_EXPR_LINK:
        if (!gen_expr(fp, np->u.link.left))
            return false;
        if (!gen_expr(fp, np->u.link.right))
            return false;
        break;
    default:
        error(&np->pos, "couldn't generate statement code");
        return false;
    }
    return true;
}

static bool gen_func(FILE *fp, SYMBOL *sym)
{
    if (!sym_is_static(sym))
        fprintf(fp, ".global %s\n", sym->id);
    if (!sym_is_extern(sym))
        fprintf(fp, "%s:\n", sym->id);
    if (sym->body) {
        int i;
        fprintf(fp, "    push rbp\n");
        fprintf(fp, "    mov rbp, rsp\n");
        fprintf(fp, "    sub rbp, %d\n", 8);/*TODO calc local var size*/
        for (i = 0; i < sym->num; i++) {
            fprintf(fp, "    mov [rbp-%d],%s\n",
                    s_param_offset32[i], s_param_reg32[i]);
            /*TODO consider type (bits) */
        }
        if (!gen_stmt(fp, sym->body))
            return false;
        fprintf(fp, "    mov rsp, rbp\n");
        fprintf(fp, "    pop rbp\n");
        fprintf(fp, "    ret\n");
    }
    return true;
}


static bool gen_data(FILE *fp, SYMBOL *sym)
{
    fprintf(fp, "%s:\n    .zero 8\n", sym->id);
    return true;
}

static bool gen_symtab(FILE *fp, SYMTAB *tab)
{
    SYMBOL *sym;
    if (tab == NULL)
        return true;
    for (sym = tab->head; sym != NULL; sym = sym->next) {
        if (sym->kind == SK_GLOBAL && !gen_data(fp, sym))
            return false;
    }
    for (sym = tab->head; sym != NULL; sym = sym->next) {
        if (sym->kind == SK_FUNC && !gen_func(fp, sym))
            return false;
    }
    return true;
}

static void gen_header(FILE *fp)
{
    fprintf(fp, ".intel_syntax noprefix\n");
}

static void gen_footer(FILE *fp)
{
}

bool generate(FILE *fp)
{
    bool result;
    gen_header(fp);
    result = gen_symtab(fp, get_global_symtab());
    gen_footer(fp);
    return result;
}
