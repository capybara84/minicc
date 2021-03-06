#ifndef minicc_h__
#define minicc_h__

#define VERSION     "0.0"
#define MAX_IDENT   256

#define BYTE_CHAR   1
#define BYTE_SHORT  2
#define BYTE_INT    4
#define BYTE_LONG   8
#define BYTE_PTR    8

#define MAX_CHAR    127
#define MIN_CHAR    -128
#define MAX_UCHAR   255
#define MAX_SHORT   32767
#define MIN_SHORT   -32768
#define MAX_USHORT  65535
#define MAX_INT     2147483647
#define MIN_INT     -2147483648
#define MAX_UINT    4294967295

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

typedef int bool;
#define false   0
#define true    1

#ifndef NDEBUG
void set_debug(const char *s);
bool is_debug(const char *s);
#endif

typedef struct {
    const char *filename;
    int line;
} POS;

int get_num_errors(void);
int get_num_warning(void);
void vwarning(const POS *pos, const char *s, va_list arg);
void verror(const POS *pos, const char *s, va_list arg);
void warning(const POS *pos, const char *s, ...);
void error(const POS *pos, const char *s, ...);

void *alloc(size_t size);
char *str_dup(const char *s);

typedef struct string {
    struct string *next;
    char *s;
    int num;
} STRING;
typedef struct string_pool {
    STRING *head;
    STRING *tail;
} STRING_POOL;

extern STRING_POOL g_string_pool;


typedef enum {
    TK_EOF, TK_ID, TK_CHAR_LIT, TK_INT_LIT, TK_UINT_LIT,
    TK_LONG_LIT, TK_ULONG_LIT, TK_FLOAT_LIT, TK_DOUBLE_LIT,
    TK_STRING_LIT, TK_TYPEDEF_NAME,
    TK_COMMA, TK_TILDE, TK_SEMI, TK_COLON, TK_QUES, TK_LPAR, TK_RPAR,
    TK_LBRA, TK_RBRA, TK_BEGIN, TK_END, TK_DOT, TK_ELLIPSIS,
    TK_ASSIGN, TK_EQ, TK_NOT, TK_NEQ, TK_STAR, TK_MUL_AS, TK_SLASH,
    TK_DIV_AS, TK_PERCENT, TK_MOD_AS, TK_HAT, TK_XOR_AS, TK_AND, TK_LAND,
    TK_AND_AS, TK_OR, TK_LOR, TK_OR_AS, TK_PLUS, TK_INC, TK_ADD_AS,
    TK_MINUS, TK_DEC, TK_SUB_AS, TK_PTR, TK_LT, TK_LEFT, TK_LEFT_AS,
    TK_LE, TK_GT, TK_RIGHT, TK_RIGHT_AS, TK_GE, TK_SIZEOF, TK_TYPEDEF,
    TK_EXTERN, TK_STATIC, TK_AUTO, TK_REGISTER,
    TK_CHAR, TK_SHORT, TK_INT, TK_LONG, TK_SIGNED, TK_UNSIGNED,
    TK_FLOAT, TK_DOUBLE, TK_CONST, TK_VOLATILE, TK_VOID,
    TK_STRUCT, TK_UNION, TK_ENUM, TK_IF, TK_ELSE, TK_SWITCH, TK_CASE,
    TK_DEFAULT, TK_WHILE, TK_DO, TK_FOR, TK_GOTO, TK_CONTINUE,
    TK_BREAK, TK_RETURN,
} TOKEN;

typedef struct {
    const char *source;
    int size;
    int current;
    int ch;
    POS pos;
    int num;
    char *id;
    STRING *str;
} SCANNER;

SCANNER *open_scanner_text(const char *filename, const char *text);
SCANNER *open_scanner_file(const char *filename);
bool close_scanner(SCANNER *scan);
bool is_next_colon(SCANNER *scan);
TOKEN next_token(SCANNER *scan);
char *intern(const char *s);
const char *token_to_string(TOKEN tk);
const char *scan_token_to_string(SCANNER *scan, TOKEN tk);

typedef enum {
    NK_COMPOUND, NK_LINK, NK_IF, NK_THEN, NK_SWITCH, NK_CASE, NK_DEFAULT,
    NK_WHILE, NK_DO, NK_FOR, NK_FOR2, NK_FOR3, NK_GOTO, NK_CONTINUE,
    NK_BREAK, NK_RETURN, NK_LABEL, NK_EXPR,
    NK_EXPR_LINK, NK_ASSIGN, NK_AS_MUL, NK_AS_DIV, NK_AS_MOD, NK_AS_ADD,
    NK_AS_SUB, NK_AS_SHL, NK_AS_SHR, NK_AS_AND, NK_AS_XOR, NK_AS_OR,
    NK_EQ, NK_NEQ, NK_LT, NK_GT, NK_LE, NK_GE, NK_SHL, NK_SHR, NK_ADD, NK_SUB,
    NK_MUL, NK_DIV, NK_MOD, NK_CAST, NK_PREINC, NK_PREDEC, NK_SIZEOF,
    NK_COND, NK_COND2, NK_LOR, NK_LAND, NK_OR, NK_XOR, NK_AND,
    NK_ARRAY, NK_CALL, NK_DOT, NK_PTR, NK_POSTINC, NK_POSTDEC, NK_ID,
    NK_CHAR_LIT, NK_INT_LIT, NK_UINT_LIT, NK_LONG_LIT, NK_ULONG_LIT,
    NK_FLOAT_LIT, NK_DOUBLE_LIT, NK_STRING_LIT, NK_ARG,
    NK_ADDR, NK_DEREF, NK_UPLUS, NK_UMINUS, NK_COMPLEMENT, NK_NOT,
} NODE_KIND;


typedef enum {
    T_UNKNOWN, T_VOID, T_NULL, T_CHAR, T_UCHAR, T_SHORT, T_USHORT,
    T_INT, T_UINT, T_LONG, T_ULONG, T_FLOAT, T_DOUBLE,
    T_STRUCT, T_UNION, T_ENUM, T_TYPEDEF_NAME,
    T_POINTER, T_ARRAY, T_FUNC,
    T_SIGNED, T_UNSIGNED,
} TYPE_KIND;

typedef enum {
    SC_DEFAULT, SC_AUTO, SC_REGISTER, SC_STATIC, SC_EXTERN, SC_TYPEDEF,
} STORAGE_CLASS;

typedef enum {
    TQ_DEFAULT      = 0x0,
    TQ_CONST        = 0x1,
    TQ_VOLATILE     = 0x2,
} TYPE_QUALIFIER;

typedef struct type {
    TYPE_KIND kind;
    STORAGE_CLASS sclass;
    TYPE_QUALIFIER tqual;
    struct type *type;
    struct param *param;
    char *tag;
    int size;
} TYPE;

extern TYPE g_type_null, g_type_uchar, g_type_int;

typedef struct param {
    struct param *next;
    char *id;
    TYPE *type;
} PARAM;

PARAM *new_param(char *id, TYPE *typ);
void add_param(PARAM *top, PARAM *param);

TYPE *new_type(TYPE_KIND kind, TYPE *typ);
TYPE *dup_type(TYPE *typ);
bool equal_type(const TYPE *tl, const TYPE *tr);
TYPE *type_check_array(const POS *pos, const TYPE *arr, const TYPE *e);
TYPE *type_check_call(const POS *pos, const TYPE *fn, const TYPE *arg);
TYPE *type_check_idnode(const POS *pos, NODE_KIND kind,
                        const TYPE *e, const char *id);
TYPE *type_check_postfix(const POS *pos, NODE_KIND kind, TYPE *e);
TYPE *type_check_unary(const POS *pos, NODE_KIND kind, TYPE *e);
TYPE *type_check_bin(const POS *pos, NODE_KIND kind, TYPE *lhs, TYPE *rhs);
void type_check_value(const POS *pos, const TYPE *t);
void type_check_integer(const POS *pos, const TYPE *t);
void type_check_assign(const POS *pos, const TYPE *lhs, const TYPE *rhs);
void type_check_assign_number(const POS *pos, const TYPE *lhs, const TYPE *rhs);
void type_check_assign_number_or_pointer(const POS *pos,
        const TYPE *lhs, const TYPE *rhs);
void type_check_assign_integer(const POS *pos, const TYPE *lhs, const TYPE *rhs);
bool is_const_type(const TYPE *t);

const char *get_type_string(const TYPE *typ);
void fprint_type(FILE *fp, const TYPE *typ);
void print_type(const TYPE *typ);

typedef enum {
    SK_LOCAL, SK_GLOBAL, SK_PARAM, SK_FUNC,
} SYMBOL_KIND;

typedef struct symbol SYMBOL;
typedef struct symtab SYMTAB;
typedef struct node NODE;

struct symbol {
    SYMBOL *next;
    SYMBOL_KIND kind;
    const char *id;
    TYPE *type;
    int scope;

    int num;    /* func: num of parameter, var: order */
    int offset; /* func: local size */

    SYMTAB *tab;
    NODE *body;
};

struct symtab {
    SYMBOL *head;
    SYMBOL *tail;
    struct symtab *up;
    int scope;
};

SYMBOL *new_symbol(SYMBOL_KIND kind, const char *id, TYPE *type, int scope);
SYMBOL *lookup_symbol(const char *id);

SYMTAB *get_global_symtab(void);
bool init_symtab(void);
void term_symtab(void);
SYMTAB *enter_scope(void);
void leave_scope(void);
void enter_function(SYMBOL *sym);
void leave_function(void);
bool sym_is_left_value(const SYMBOL *sym);
bool sym_is_static(const SYMBOL *sym);
bool sym_is_extern(const SYMBOL *sym);

int get_current_func_local_offset(void);

void fprint_func_comment(FILE *fp, const SYMBOL *sym);
void fprint_symbol(FILE *fp, int indent, const SYMBOL *sym);
void fprint_symtab(FILE *fp, int indent, const SYMTAB *tab);
void print_symtab(const SYMTAB *tab);
void print_global_symtab(void);


struct node {
    NODE_KIND kind;
    POS pos;
    TYPE *type;
    union {
        struct {
            NODE *left;
            NODE *right;
        } link;
        struct {
            NODE *node;
            SYMTAB *symtab;
        } comp;
        struct {
            NODE *node;
            const char *id;
        } idnode;
        struct {
            NODE *node;
            int num;
        } num_node;
        STRING *str;
        int num;
        const char *id;
        SYMBOL *sym;
    } u;
};

NODE *new_node(NODE_KIND kind, const POS *pos, TYPE *typ);
NODE *new_node1(NODE_KIND kind, const POS *pos, TYPE *typ, NODE *np);
NODE *new_node2(NODE_KIND kind, const POS *pos, TYPE *typ, NODE *left, NODE *right);
NODE *new_node_num(NODE_KIND kind, const POS *pos, TYPE *typ, int num);
NODE *new_node_id(NODE_KIND kind, const POS *pos, TYPE *typ, const char *id);
NODE *new_node_sym(NODE_KIND kind, const POS *pos, TYPE *typ, SYMBOL *sym);
NODE *new_node_idnode(NODE_KIND kind, const POS *pos, TYPE *typ,
        NODE *ep, const char *id);
NODE *new_node_num_node(NODE_KIND kind, const POS *pos, TYPE *typ,
        NODE *np, int num);
NODE *new_node_string(const POS *pos, STRING *str);
NODE *node_link(NODE_KIND kind, const POS *pos, NODE *n, NODE *top);
bool calc_constant_expr(const NODE *np, int *result);
const char *get_node_op_string(NODE_KIND kind);
void fprint_node(FILE *fp, int indent, const NODE *np);
void print_node(const NODE *np);

typedef struct {
    SCANNER *scan;
    TOKEN token;
} PARSER;

PARSER *open_parser_text(const char *filename, const char *text);
PARSER *open_parser(const char *filename);
bool close_parser(PARSER *pars);
bool parse(PARSER *pars);

bool generate(FILE *fp);

#endif
