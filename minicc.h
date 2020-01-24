#ifndef minicc_h__
#define minicc_h__

#define VERSION     "0.0"
#define MAX_IDENT   256

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
void error(const POS *pos, const char *s, ...);

void *alloc(size_t size);
char *str_dup(const char *s);

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
    T_UNKNOWN, T_VOID, T_NULL, T_CHAR, T_UCHAR, T_SHORT, T_USHORT,
    T_INT, T_UINT, T_LONG, T_ULONG, T_FLOAT, T_DOUBLE,
    T_STRUCT, T_UNION, T_ENUM, T_TYPEDEF_NAME,
    T_POINTER, T_ARRAY, T_FUNC,
    T_SIGNED, T_UNSIGNED,
} TYPE_KIND;

typedef struct type {
    TYPE_KIND kind;
    bool is_const;
    struct type *type;
    struct param *param;
    char *tag;
} TYPE;

typedef struct param {
    struct param *next;
    char *id;
    TYPE *type;
    bool is_register;
} PARAM;

PARAM *new_param(char *id, TYPE *typ);
void add_param(PARAM *top, PARAM *param);

TYPE *new_type(TYPE_KIND kind, TYPE *typ);
TYPE *dup_type(TYPE *typ);
bool equal_type(const TYPE *tl, const TYPE *tr);

const char *get_type_string(const TYPE *typ);
void fprint_type(FILE *fp, const TYPE *typ);
void print_type(const TYPE *typ);

typedef enum {
    SC_DEFAULT, SC_AUTO, SC_REGISTER, SC_STATIC, SC_EXTERN, SC_TYPEDEF,
} STORAGE_CLASS;

typedef enum {
    TQ_DEFAULT, TQ_CONST, TQ_VOLATILE,
} TYPE_QUALIFIER;

typedef enum {
    SK_LOCAL, SK_GLOBAL, SK_PARAM, SK_FUNC,
} SYMBOL_KIND;

typedef struct symbol SYMBOL;
typedef struct symtab SYMTAB;
typedef struct node NODE;

struct symbol {
    SYMBOL *next;
    STORAGE_CLASS sclass;
    SYMBOL_KIND kind;
    bool is_volatile;
    const char *id;
    TYPE *type;
    SYMTAB *tab;
    NODE *body;
};

struct symtab {
    SYMBOL *head;
    SYMBOL *tail;
    struct symtab *up;
};

SYMBOL *new_symbol(SYMBOL_KIND kind, STORAGE_CLASS sc, TYPE_QUALIFIER tq,
                    const char *id, TYPE *type);
SYMBOL *lookup_symbol(const char *id);

bool init_symtab(void);
void term_symtab(void);
SYMTAB *enter_scope(void);
void leave_scope(void);
void enter_function(SYMBOL *sym);
void leave_function(void);
void fprint_symbol(FILE *fp, int indent, const SYMBOL *sym);
void fprint_symtab(FILE *fp, int indent, const SYMTAB *tab);
void print_symtab(const SYMTAB *tab);
void print_global_symtab(void);

typedef enum {
    NK_COMPOUND,
    NK_LINK,
    NK_CASE,
    NK_DEFAULT,
    NK_IF,
    NK_THEN,
    NK_SWITCH,
    NK_WHILE,
    NK_DO,
    NK_FOR,
    NK_FOR2,
    NK_FOR3,
    NK_GOTO,
    NK_CONTINUE,
    NK_BREAK,
    NK_RETURN,
    NK_EXPR,
    NK_EXPR_LINK,
} NODE_KIND;

struct node {
    NODE_KIND kind;
    POS pos;
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
            int num;
            NODE *body;
        } ncase;
        int num;
        const char *id;
    } u;
};

NODE *new_node(NODE_KIND kind, const POS *pos);
NODE *new_node1(NODE_KIND kind, const POS *pos, NODE *np);
NODE *new_node2(NODE_KIND kind, const POS *pos, NODE *left, NODE *right);
NODE *new_node_num(NODE_KIND kind, const POS *pos, int num);
NODE *new_node_id(NODE_KIND kind, const POS *pos, const char *id);
NODE *new_node_case(NODE_KIND kind, const POS *pos, int num, NODE *body);
NODE *node_link(NODE_KIND kind, const POS *pos, NODE *n, NODE *top);
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

#endif
