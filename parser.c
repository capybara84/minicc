#include "minicc.h"

TYPE *new_unknown_type(void)
{
    TYPE *tp = (TYPE*) alloc(sizeof (TYPE));
    tp->kind = T_UNKNOWN;
    tp->is_const = false;
    tp->type = NULL;
    tp->param = NULL;
    return tp;
}

NODE *new_node(NODE_KIND kind, const POS *pos)
{
    NODE *np = (NODE*) alloc(sizeof (NODE));
    np->kind = kind;
    np->pos = *pos;
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

void fprint_node(FILE *fp, const NODE *np)
{
    if (np == NULL)
        return;
    switch (np->kind) {
    case NK_EXTERNAL_DECL:
        break;
    }
}

void print_node(const NODE *np)
{
    fprint_node(stdout, np);
}

#define COUNT_OF(array) (sizeof (array) / sizeof (array[0]))

#ifdef NDEBUG
# define ENTER(fn)      ((void) 0)
# define LEAVE(fn)      ((void) 0)
# define TRACE(fn)      ((void) 0)
#else
static int s_indent = 0;
# define ENTER(fn)      if (is_debug("parser_trace")) \
                            printf("%*sENTER %s\n", s_indent++, "", (fn))
# define LEAVE(fn)      if (is_debug("parser_trace")) \
                            printf("%*sLEAVE %s\n", --s_indent, "", (fn))
# define TRACE(fn,s)    if (is_debug("parser_trace")) \
                            printf("%*sTRACE %s %s\n", s_indent, "", (fn), (s))
#endif

const POS *get_pos(const PARSER *pars)
{
    return &pars->scan->pos;
}


#ifdef NDEBUG
#define next(pars)  ((pars)->token = next_token((pars)->scan))
#else
static TOKEN next(PARSER *pars)
{
    assert(pars);
    assert(pars->scan);

    pars->token = next_token(pars->scan);
    if (is_debug("parser")) {
        printf("%s(%d): %s\n", pars->scan->pos.filename, pars->scan->pos.line,
            scan_token_to_string(pars->scan, pars->token));
    }
    return pars->token;
}
#endif

PARSER *open_parser_text(const char *filename, const char *text)
{
    PARSER *pars = (PARSER*) alloc(sizeof (PARSER));
    pars->scan = open_scanner_text(filename, text);
    if (pars->scan == NULL) {
        free(pars);
        return NULL;
    }
    pars->token = TK_EOF;
    return pars;
}

PARSER *open_parser(const char *filename)
{
    PARSER *pars = (PARSER*) alloc(sizeof (PARSER));
    pars->scan = open_scanner_file(filename);
    if (pars->scan == NULL) {
        free(pars);
        return NULL;
    }
    pars->token = TK_EOF;
    return pars;
}

bool close_parser(PARSER *pars)
{
    if (pars == NULL)
        return false;
    close_scanner(pars->scan);
    free(pars);
    return true;
}

static bool is_token(PARSER *pars, TOKEN tok)
{
    assert(pars);

    if (pars->token == tok)
        return true;
    return false;
}

static void skip_error(PARSER *pars)
{
    assert(pars);

    while (!is_token(pars, TK_SEMI) && !is_token(pars, TK_EOF))
        next(pars);
}

static void parser_error(PARSER *pars, const char *s, ...)
{
    char buffer[512];
    char buffer2[512];
    const char *token_name;
    va_list ap;

    assert(pars);
    assert(pars->scan);
    assert(s);

    va_start(ap, s);
    vsprintf(buffer, s, ap);
    va_end(ap);

    token_name = scan_token_to_string(pars->scan, pars->token);
    sprintf(buffer2, "%s at token %s", buffer, token_name);

    error(&pars->scan->pos, buffer2);

    skip_error(pars);
}

static void parser_warning(PARSER *pars, const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    vwarning(&pars->scan->pos, s, ap);
    va_end(ap);
}

static bool is_token_with_array(PARSER *pars, TOKEN begin_with[], int count)
{
    TOKEN token;
    int i;

    assert(pars);
    token = pars->token;

    for (i = 0; i < count; i++)
        if (token == begin_with[i])
            return true;
    return false;
}

static bool expect(PARSER *pars, TOKEN tok)
{
    assert(pars);

    if (is_token(pars, tok)) {
        next(pars);
        return true;
    }
    parser_error(pars, "missing token %s", token_to_string(tok));
    return false;
}

static bool expect_id(PARSER *pars)
{
    assert(pars);
    if (is_token(pars, TK_ID))
        return true;
    parser_error(pars, "missing IDENTIFIER");
    return false;
}

static bool is_declarator(PARSER *pars)
{
    static TOKEN begin_with[] = { TK_STAR, TK_ID, TK_LPAR };

    assert(pars);

    return is_token_with_array(pars, begin_with, COUNT_OF(begin_with));
}

static bool is_declaration_specifier(PARSER *pars)
{
    static TOKEN begin_with[] = { TK_AUTO, TK_REGISTER, TK_STATIC,
            TK_EXTERN, TK_TYPEDEF,
            TK_VOID, TK_CHAR, TK_SHORT, TK_INT, TK_LONG, TK_FLOAT,
            TK_DOUBLE, TK_SIGNED, TK_UNSIGNED, TK_STRUCT, TK_UNION,
            TK_ENUM, TK_TYPEDEF_NAME, TK_CONST, TK_VOLATILE };

    assert(pars);

    return is_token_with_array(pars, begin_with, COUNT_OF(begin_with));
}

#define is_declaration_specifiers(p)    is_declaration_specifier(p)
#define is_declaration(p)               is_declaration_specifiers(p)
#define is_parameter_list(p)            is_declaration_specifiers(p)
#define is_parameter_type_list(p)       is_parameter_list(p)
#define is_init_declarator(p)           is_declarator(p)
#define is_init_declarator_list(p)      is_init_declarator(p)

static bool is_expression(PARSER *pars)
{
    static TOKEN begin_with[] = {
        TK_INC, TK_DEC, TK_SIZEOF,
        TK_AND, TK_STAR, TK_PLUS, TK_MINUS, TK_TILDE, TK_NOT,
        TK_ID, TK_CHAR_LIT, TK_INT_LIT, TK_UINT_LIT, TK_LONG_LIT,
        TK_ULONG_LIT, TK_FLOAT_LIT, TK_DOUBLE_LIT, TK_STRING_LIT,
        TK_LPAR,
    };
    assert(pars);
    return is_token_with_array(pars, begin_with, COUNT_OF(begin_with));
}

#define is_constant_expression(p)   is_expression(p)

static bool is_statement(PARSER *pars)
{
    static TOKEN begin_with[] = {
        TK_ID, TK_CASE, TK_DEFAULT, TK_SEMI,
        TK_BEGIN, TK_IF, TK_SWITCH, TK_WHILE, TK_DO, TK_FOR,
        TK_GOTO, TK_CONTINUE, TK_BREAK, TK_RETURN,
        TK_INC, TK_DEC, TK_SIZEOF,
        TK_AND, TK_STAR, TK_PLUS, TK_MINUS, TK_TILDE, TK_NOT,
        TK_CHAR_LIT, TK_INT_LIT, TK_UINT_LIT, TK_LONG_LIT,
        TK_ULONG_LIT, TK_FLOAT_LIT, TK_DOUBLE_LIT, TK_STRING_LIT,
        TK_LPAR,
    };
    assert(pars);
    return is_token_with_array(pars, begin_with, COUNT_OF(begin_with));
}

static bool is_assignment_operator(PARSER *pars)
{
    static TOKEN begin_with[] = {
        TK_ASSIGN, TK_MUL_AS, TK_DIV_AS, TK_MOD_AS, TK_ADD_AS,
        TK_SUB_AS, TK_LEFT_AS, TK_RIGHT_AS, TK_AND_AS, TK_XOR_AS,
        TK_OR_AS,
    };
    assert(pars);
    return is_token_with_array(pars, begin_with, COUNT_OF(begin_with));
}

static bool is_type_specifier_or_qualifier(PARSER *pars)
{
    static TOKEN begin_with[] = {
        TK_VOID, TK_CHAR, TK_SHORT, TK_INT, TK_LONG, TK_FLOAT,
        TK_DOUBLE, TK_SIGNED, TK_UNSIGNED, TK_STRUCT, TK_UNION,
        TK_ENUM, TK_TYPEDEF_NAME, TK_CONST, TK_VOLATILE,
    };
    assert(pars);
    return is_token_with_array(pars, begin_with, COUNT_OF(begin_with));
}

static bool is_abstract_declarator(PARSER *pars)
{
    static TOKEN begin_with[] = {
        TK_STAR, TK_LPAR,
    };
    assert(pars);
    return is_token_with_array(pars, begin_with, COUNT_OF(begin_with));
}

static bool is_unary_operator(PARSER *pars)
{
    static TOKEN begin_with[] = {
        TK_AND, TK_STAR, TK_PLUS, TK_MINUS, TK_TILDE, TK_NOT,
    };
    assert(pars);
    return is_token_with_array(pars, begin_with, COUNT_OF(begin_with));
}

#define is_specifier_qualifier_list(p)  is_type_specifier_or_qualifier(p)
#define is_struct_declaration(p)        is_specifier_qualifier_list(p)


static bool parse_assignment_expression(PARSER *pars);

/*
argument_expression_list
    = assignment_expression {',' assignment_expression}
*/
static bool parse_argument_expression_list(PARSER *pars)
{
    ENTER("parse_argument_expression_list");
    assert(pars);
    
    if (!parse_assignment_expression(pars))
        return false;
    while (is_token(pars, TK_COMMA)) {
        next(pars);
        if (!parse_assignment_expression(pars))
            return false;
    }
    LEAVE("parse_argument_expression_list");
    return true;
}

static bool parse_expression(PARSER *pars);

/*
primary_expression
    = IDENTIFIER
    | constant
    | STRING
    | '(' expression ')'
constant
    = INTEGER_CONSTANT
    | CHARACTER_CONSTANT
    | FLOATING_CONSTANT
    | ENUMERATION_CONSTANT
*/
static bool parse_primary_expression(PARSER *pars)
{
    ENTER("parse_primary_expression");
    assert(pars);

    switch (pars->token) {
    case TK_ID:
        TRACE("parse_primary_expression", "ID");
        next(pars);
        break;
    case TK_INT_LIT:
    case TK_UINT_LIT:
    case TK_LONG_LIT:
    case TK_ULONG_LIT:
    case TK_CHAR_LIT:
    case TK_FLOAT_LIT:
    case TK_DOUBLE_LIT:
    case TK_STRING_LIT:
        TRACE("parse_primary_expression", "_LIT");
        next(pars);
        break;
    case TK_LPAR:
        TRACE("parse_primary_expression", "()");
        next(pars);
        if (!parse_expression(pars))
            return false;
        if (!expect(pars, TK_RPAR))
            return false;
        break;
    default:
        parser_error(pars, "syntax error (expression)");
        return false;
    }

    LEAVE("parse_primary_expression");
    return true;
}

/*
postfix_expression
    = primary_expression
    | postfix_expression '[' expression ']'
    | postfix_expression '(' [argument_expression_list] ')'
    | postfix_expression '.' IDENTIFIER
    | postfix_expression '->' IDENTIFIER
    | postfix_expression '++'
    | postfix_expression '--'
*/
static bool parse_postfix_expression(PARSER *pars)
{
    ENTER("parse_postfix_expression");
    assert(pars);

    if (!parse_primary_expression(pars))
        return false;
    switch (pars->token) {
    case TK_LBRA:
        next(pars);
        if (!parse_expression(pars))
            return false;
        if (!expect(pars, TK_RBRA))
            return false;
        break;
    case TK_LPAR:
        next(pars);
        if (!is_token(pars, TK_RPAR)) {
            if (!parse_argument_expression_list(pars))
                return false;
        }
        if (!expect(pars, TK_RPAR))
            return false;
        break;
    case TK_DOT:
        next(pars);
        if (!expect_id(pars))
            return false;
        next(pars);
        break;
    case TK_PTR:
        next(pars);
        if (!expect_id(pars))
            return false;
        next(pars);
        break;
    case TK_INC:
        next(pars);
        break;
    case TK_DEC:
        next(pars);
        break;
    default:
        break;
    }
    LEAVE("parse_postfix_expression");
    return true;
}

static bool parse_type_name(PARSER *pars);
static bool parse_cast_expression(PARSER *pars);

/*
unary_expression
    = postfix_expression
    | '++' unary_expression
    | '--' unary_expression
    | unary_operator cast_expression
    | SIZEOF unary_expression
    | SIZEOF '(' type_name ')'
*/
static bool parse_unary_expression(PARSER *pars)
{
    ENTER("parse_unary_expression");
    assert(pars);

    if (is_token(pars, TK_INC) || is_token(pars, TK_DEC)) {
        next(pars);
        if (!parse_unary_expression(pars))
            return false;
    } else if (is_unary_operator(pars)) {
        next(pars);
        if (!parse_cast_expression(pars))
            return false;
    } else if (is_token(pars, TK_SIZEOF)) {
        next(pars);
        if (is_token(pars, TK_LPAR)) {
            next(pars);
            if (!parse_type_name(pars))
                return false;
            if (!expect(pars, TK_RPAR))
                return false;
        } else {
            if (!parse_unary_expression(pars))
                return false;
        }
    } else {
        if (!parse_postfix_expression(pars))
            return false;
    }

    LEAVE("parse_unary_expression");
    return true;
}

/*
cast_expression
    = unary_expression
    | '(' type_name ')' cast_expression
*/
static bool parse_cast_expression(PARSER *pars)
{
    ENTER("parse_cast_expression");
    assert(pars);

    while (is_token(pars, TK_LPAR)) {
        next(pars);
        if (!parse_type_name(pars))
            return false;
        if (!expect(pars, TK_RPAR))
            return false;
    }
    if (!parse_unary_expression(pars))
        return false;
    LEAVE("parse_cast_expression");
    return true;
}

/*
multiplicative_expression
	= cast_expression {('*'|'/'|'%') cast_expression}
*/
static bool parse_multiplicative_expression(PARSER *pars)
{
    ENTER("parse_multiplicative_expression");
    assert(pars);

    if (!parse_cast_expression(pars))
        return false;
    while (is_token(pars, TK_STAR) || is_token(pars, TK_SLASH)
            || is_token(pars, TK_PERCENT)) {
        next(pars);
        if (!parse_cast_expression(pars))
            return false;
    }
    LEAVE("parse_multiplicative_expression");
    return true;
}

/*
additive_expression
	= multiplicative_expression {('+'|'-') multiplicative_expression}
*/
static bool parse_additive_expression(PARSER *pars)
{
    ENTER("parse_additive_expression");
    assert(pars);

    if (!parse_multiplicative_expression(pars))
        return false;
    while (is_token(pars, TK_PLUS) || is_token(pars, TK_MINUS)) {
        next(pars);
        if (!parse_multiplicative_expression(pars))
            return false;
    }
    LEAVE("parse_additive_expression");
    return true;
}

/*
shift_expression
	= additive_expression {('<<'|'>>') additive_expression}
*/
static bool parse_shift_expression(PARSER *pars)
{
    ENTER("parse_shift_expression");
    assert(pars);

    if (!parse_additive_expression(pars))
        return false;
    while (is_token(pars, TK_LEFT) || is_token(pars, TK_RIGHT)) {
        next(pars);
        if (!parse_additive_expression(pars))
            return false;
    }
    LEAVE("parse_shift_expression");
    return true;
}

/*
relational_expression
	= shift_expression {('<'|'>'|'<='|'>=') shift_expression}
*/
static bool parse_relational_expression(PARSER *pars)
{
    TOKEN shift_op[] = { TK_LT, TK_GT, TK_LE, TK_GE };
    ENTER("parse_relational_expression");
    assert(pars);

    if (!parse_shift_expression(pars))
        return false;
    while (is_token_with_array(pars, shift_op, COUNT_OF(shift_op))) {
        next(pars);
        if (!parse_shift_expression(pars))
            return false;
    }
    LEAVE("parse_relational_expression");
    return true;
}

/*
equality_expression
	= relational_expression {('=='|'!=') relational_expression}
*/
static bool parse_equality_expression(PARSER *pars)
{
    ENTER("parse_equality_expression");
    assert(pars);

    if (!parse_relational_expression(pars))
        return false;
    while (is_token(pars, TK_EQ) || is_token(pars, TK_NEQ)) {
        next(pars);
        if (!parse_relational_expression(pars))
            return false;
    }
    LEAVE("parse_equality_expression");
    return true;
}

/*
and_expression
	= equality_expression {'&' equality_expression}
*/
static bool parse_and_expression(PARSER *pars)
{
    ENTER("parse_and_expression");
    assert(pars);

    if (!parse_equality_expression(pars))
        return false;
    while (is_token(pars, TK_AND)) {
        next(pars);
        if (!parse_equality_expression(pars))
            return false;
    }
    LEAVE("parse_and_expression");
    return true;
}

/*
exclusive_or_expression
	= and_expression {'^' and_expression}
*/
static bool parse_exclusive_or_expression(PARSER *pars)
{
    ENTER("parse_exclusive_or_expression");
    assert(pars);

    if (!parse_and_expression(pars))
        return false;
    while (is_token(pars, TK_HAT)) {
        next(pars);
        if (!parse_and_expression(pars))
            return false;
    }
    LEAVE("parse_exclusive_or_expression");
    return true;
}

/*
inclusive_or_expression
	= exclusive_or_expression {'|' exclusive_or_expression}
*/
static bool parse_inclusive_or_expression(PARSER *pars)
{
    ENTER("parse_inclusive_or_expression");
    assert(pars);

    if (!parse_exclusive_or_expression(pars))
        return false;
    while (is_token(pars, TK_OR)) {
        next(pars);
        if (!parse_exclusive_or_expression(pars))
            return false;
    }
    LEAVE("parse_inclusive_or_expression");
    return true;
}

/*
logical_and_expression
	= inclusive_or_expression {'&&' inclusive_or_expression}
*/
static bool parse_logical_and_expression(PARSER *pars)
{
    ENTER("parse_logical_and_expression");
    assert(pars);

    if (!parse_inclusive_or_expression(pars))
        return false;
    while (is_token(pars, TK_LAND)) {
        next(pars);
        if (!parse_inclusive_or_expression(pars))
            return false;
    }
    LEAVE("parse_logical_and_expression");
    return true;
}

/*
logical_or_expression
	= logical_and_expression {'||' logical_and_expression}
*/
static bool parse_logical_or_expression(PARSER *pars)
{
    ENTER("parse_logical_or_expression");
    assert(pars);

    if (!parse_logical_and_expression(pars))
        return false;
    while (is_token(pars, TK_LOR)) {
        next(pars);
        if (!parse_logical_and_expression(pars))
            return false;
    }
    LEAVE("parse_logical_or_expression");
    return true;
}

/*
conditional_expression
    = logical_or_expression ['?' expression ':' conditional_expression]
*/
static bool parse_conditional_expression(PARSER *pars)
{
    ENTER("parse_conditional_expression");

    assert(pars);

    if (!parse_logical_or_expression(pars))
        return false;
    if (is_token(pars, TK_QUES)) {
        next(pars);
        if (!parse_expression(pars))
            return false;
        if (!expect(pars, TK_SEMI))
            return false;
        if (!parse_conditional_expression(pars))
            return false;
    }
    LEAVE("parse_conditional_expression");
    return true;
}

/*
assignment_expression
	= conditional_expression
	| unary_expression assignment_operator assignment_expression
*/
static bool parse_assignment_expression(PARSER *pars)
{
    ENTER("parse_assignment_expression");
    assert(pars);

    if (!parse_conditional_expression(pars))
        return false;
    if (is_assignment_operator(pars)) {
        next(pars);
        if (!parse_assignment_expression(pars))
            return false;
    }
    LEAVE("parse_assignment_expression");
    return true;
}

/*
expression
	= assignment_expression {',' assignment_expression}
*/
static bool parse_expression(PARSER *pars)
{
    ENTER("parse_expression");
    assert(pars);

    if (!parse_assignment_expression(pars))
        return false;
    while (is_token(pars, TK_COMMA)) {
        next(pars);
        if (!parse_assignment_expression(pars))
            return false;
    }
    LEAVE("parse_expression");
    return true;
}

/*
constant_expression
    = conditional_expression
*/
static bool parse_constant_expression(PARSER *pars)
{
    ENTER("parse_constant_expression");

    assert(pars);

    if (!parse_conditional_expression(pars))
        return false;

    LEAVE("parse_constant_expression");
    return true;
}

static bool parse_compound_statement(PARSER *pars);

/*
statement
	= labeled_statement
	| expression_statement
	| compound_statement
	| selection_statement
	| iteration_statement
	| jump_statement

labeled_statement
	= IDENTIFIER ':' statement
	| CASE constant_expression ':' statement
	| DEFAULT ':' statement

expression_statement
	= [expression] ';'

compound_statement
	= '{' {declaration} {statement} '}'

selection_statement
	= IF '(' expression ')' statement
	| IF '(' expression ')' statement ELSE statement
	| SWITCH '(' expression ')' statement

iteration_statement
	= WHILE '(' expression ')' statement
	| DO statement WHILE '(' expression ')' ';'
	| FOR '(' [expression] ';' [expression] ';' [expression] ')'
		statement

jump_statement
	= GOTO ID ';'
	| CONTINUE ';'
	| BREAK ';'
	| RETURN [expression] ';'

*/
static bool parse_statement(PARSER *pars)
{
    ENTER("parse_statement");
    assert(pars);
    
    switch (pars->token) {
    case TK_CASE:
        TRACE("parse_statement", "case");
        next(pars);
        if (!parse_constant_expression(pars))
            return false;
        if (!expect(pars, TK_COLON))
            return false;
        if (!parse_statement(pars))
            return false;
        break;
    case TK_DEFAULT:
        TRACE("parse_statement", "default");
        next(pars);
        if (!expect(pars, TK_COLON))
            return false;
        if (!parse_statement(pars))
            return false;
        break;
    case TK_BEGIN:
        TRACE("parse_statement", "compound");
        if (!parse_compound_statement(pars))
            return false;
        break;
    case TK_IF:
        TRACE("parse_statement", "if");
        next(pars);
        if (!expect(pars, TK_LPAR))
            return false;
        if (!parse_expression(pars))
            return false;
        if (!expect(pars, TK_RPAR))
            return false;
        if (!parse_statement(pars))
            return false;
        if (is_token(pars, TK_ELSE)) {
            next(pars);
            if (!parse_statement(pars))
                return false;
        }
        break;
    case TK_SWITCH:
        TRACE("parse_statement", "switch");
        next(pars);
        if (!expect(pars, TK_LPAR))
            return false;
        if (!parse_expression(pars))
            return false;
        if (!expect(pars, TK_RPAR))
            return false;
        if (!parse_statement(pars))
            return false;
        break;
    case TK_WHILE:
        TRACE("parse_statement", "while");
        next(pars);
        if (!expect(pars, TK_LPAR))
            return false;
        if (!parse_expression(pars))
            return false;
        if (!expect(pars, TK_RPAR))
            return false;
        if (!parse_statement(pars))
            return false;
        break;
    case TK_DO:
        TRACE("parse_statement", "do");
        next(pars);
        if (!parse_statement(pars))
            return false;
        if (!expect(pars, TK_WHILE))
            return false;
        if (!expect(pars, TK_LPAR))
            return false;
        if (!parse_expression(pars))
            return false;
        if (!expect(pars, TK_RPAR))
            return false;
        if (!expect(pars, TK_SEMI))
            return false;
        break;
    case TK_FOR:
        TRACE("parse_statement", "for");
        next(pars);
        if (!expect(pars, TK_LPAR))
            return false;
        if (is_expression(pars)) {
            if (!parse_expression(pars))
                return false;
        }
        if (!expect(pars, TK_SEMI))
            return false;
        if (is_expression(pars)) {
            if (!parse_expression(pars))
                return false;
        }
        if (!expect(pars, TK_SEMI))
            return false;
        if (is_expression(pars)) {
            if (!parse_expression(pars))
                return false;
        }
        if (!expect(pars, TK_RPAR))
            return false;
        if (!parse_statement(pars))
            return false;
        break;
    case TK_GOTO:
        TRACE("parse_statement", "goto");
        next(pars);
        if (!expect(pars, TK_ID))
            return false;
        if (!expect(pars, TK_SEMI))
            return false;
        break;
    case TK_CONTINUE:
        TRACE("parse_statement", "continue");
        next(pars);
        if (!expect(pars, TK_SEMI))
            return false;
        break;
    case TK_BREAK:
        TRACE("parse_statement", "break");
        next(pars);
        if (!expect(pars, TK_SEMI))
            return false;
        break;
    case TK_RETURN:
        TRACE("parse_statement", "return");
        next(pars);
        if (is_expression(pars)) {
            if (!parse_expression(pars))
                return false;
        }
        if (!expect(pars, TK_SEMI))
            return false;
        break;
    case TK_ID:
        if (is_next_colon(pars->scan)) {
            TRACE("parse_statement", "label");
            next(pars);
            if (!expect(pars, TK_COLON))
                return false;
            break;
        }
        /*THROUGH*/
    default:
        TRACE("parse_statement", "expression");
        if (pars->token != TK_SEMI) {
            if (!parse_expression(pars))
                return false;
        }
        if (!expect(pars, TK_SEMI))
            return false;
        break;
    }
    LEAVE("parse_statement");
    return true;
}

static bool parse_declaration(PARSER *pars, bool parametered);

/*
compound_statement
	= '{' {declaration} {statement} '}'
*/
static bool parse_compound_statement(PARSER *pars)
{
    ENTER("parse_compound_statement");

    assert(pars);
    if (!expect(pars, TK_BEGIN))
        return false;
    while (is_declaration(pars)) {
        if (!parse_declaration(pars, false))
            return false;
    }
    while (is_statement(pars)) {
        if (!parse_statement(pars))
            return false;
    }
    if (!expect(pars, TK_END))
        return false;
    LEAVE("parse_compound_statement");
    return true;
}

static bool parse_initializer(PARSER *pars);

/*
initializer_list
    = initializer {',' initializer}
*/
static bool parse_initializer_list(PARSER *pars)
{
    ENTER("parse_initializer_list");
    assert(pars);
    if (!parse_initializer(pars))
        return false;
    while (is_token(pars, TK_COMMA)) {
        next(pars);
        if (!parse_initializer(pars))
            return false;
    }
    LEAVE("parse_initializer_list");
    return true;
}

/*
initializer
    = assignment_expression
    | '{' initializer_list [','] '}'
*/
static bool parse_initializer(PARSER *pars)
{
    ENTER("parse_initializer");

    assert(pars);

    if (is_token(pars, TK_BEGIN)) {
        next(pars);
        if (!parse_initializer_list(pars))
            return false;
        if (is_token(pars, TK_COMMA))
            next(pars);
        if (!expect(pars, TK_END))
            return false;
    } else {
        if (!parse_assignment_expression(pars))
            return false;
    }

    LEAVE("parse_initializer");
    return true;
}

static bool parse_declarator(PARSER *pars);

/*
init_declarator
	= declarator ['=' initializer]
*/
static bool parse_init_declarator(PARSER *pars)
{
    ENTER("parse_init_declarator");

    assert(pars);

    if (!parse_declarator(pars))
        return false;
    if (is_token(pars, TK_ASSIGN)) {
        next(pars);
        if (!parse_initializer(pars))
            return false;
    }

    LEAVE("parse_init_declarator");
    return true;
}

/*
init_declarator_list
    = init_declarator {',' init_declarator}
*/
static bool parse_init_declarator_list(PARSER *pars)
{
    ENTER("parse_init_declarator_list");

    assert(pars);

    for (;;) {
        if (!parse_init_declarator(pars))
            return false;
        if (!is_token(pars, TK_COMMA))
            break;
        next(pars);
    }
    LEAVE("parse_init_declarator_list");
    return true;
}

static bool parse_declaration_specifiers(PARSER *pars,
                bool file_scoped, bool parametered,
                TYPE **typ, STORAGE_CLASS *psc, TYPE_QUALIFIER *ptq);

/*
declaration
	= declaration_specifiers [init_declarator_list] ';'
*/
static bool parse_declaration(PARSER *pars, bool parametered)
{
    TYPE *typ = new_unknown_type();
    STORAGE_CLASS sc = SC_DEFAULT;
    TYPE_QUALIFIER tq = TQ_DEFAULT;

    ENTER("parse_declaration");

    assert(pars);

    if (!parse_declaration_specifiers(pars, false,
                    parametered, &typ, &sc, &tq)) {
        return false;
    }
    if (is_init_declarator_list(pars)) {
        if (!parse_init_declarator_list(pars))
            return false;
    }
    if (!expect(pars, TK_SEMI))
        return false;

    LEAVE("parse_declaration");
    return true;
}

/*
type_qualifier_list
    = {type_qualifier}
type_qualifier
	= CONST | VOLATILE
*/
static bool parse_type_qualifier_list(PARSER *pars)
{
    ENTER("parse_type_qualifier_list");
    assert(pars);
    while (is_token(pars, TK_CONST) || is_token(pars, TK_VOLATILE)) {
        next(pars);
    }
    LEAVE("parse_type_qualifier_list");
    return true;
}

/*
pointer
	= '*' type_qualifier_list {'*' type_qualifier_list}
*/
static bool parse_pointer(PARSER *pars)
{
    ENTER("parse_pointer");

    assert(pars);

    next(pars); /* skip '*' */

    if (!parse_type_qualifier_list(pars))
        return false;
    while (is_token(pars, TK_STAR)) {
        next(pars);
        if (!parse_type_qualifier_list(pars))
            return false;
    }

    LEAVE("parse_pointer");
    return true;
}


/*
identifier_list
    = IDENTIFIER {',' IDENTIFIER}
*/
static bool parse_identifier_list(PARSER *pars)
{
    ENTER("parse_identifier_list");

    assert(pars);
    next(pars); /* skip ID */
    while (is_token(pars, TK_COMMA)) {
        next(pars);
        if (!expect(pars, TK_ID))
            return false;
    }

    LEAVE("parse_identifier_list");
    return true;
}

/*
type_specifier_or_qualifier
    = type_specifier | type_qualifier
type_specifier
	= VOID | CHAR | SHORT | INT | LONG | FLOAT | DOUBLE
	| SIGNED | UNSIGNED | struct_or_union_specifier
	| enum_specifier | typedef_name
type_qualifier
	= CONST | VOLATILE
*/
static bool parse_type_specifier_or_qualifier(PARSER *pars)
{
    ENTER("parse_type_specifier_or_qualifier");
    assert(pars);

    switch (pars->token) {
    case TK_VOID:
    case TK_CHAR:
    case TK_SHORT:
    case TK_INT:
    case TK_LONG:
    case TK_FLOAT:
    case TK_DOUBLE:
    case TK_SIGNED:
    case TK_UNSIGNED:
    case TK_STRUCT:
    case TK_UNION:
    case TK_ENUM:
    case TK_TYPEDEF_NAME:
        next(pars);
        break;
    case TK_CONST:
    case TK_VOLATILE:
        next(pars);
        break;
    default:
        assert(0);
        return false;
    }
    LEAVE("parse_type_specifier_or_qualifier");
    return true;
}

/*
specifier_qualifier_list
    = type_specifier_or_qualifier {type_specifier_or_qualifier}
*/
static bool parse_specifier_qualifier_list(PARSER *pars)
{
    ENTER("parse_specifier_qualifier_list");
    assert(pars);

    if (!is_type_specifier_or_qualifier(pars)) {
        parser_error(pars, "missing type_specifier or type_qualifier");
        return false;
    }
    if (!parse_type_specifier_or_qualifier(pars))
        return false;
    while (is_type_specifier_or_qualifier(pars)) {
        if (!parse_type_specifier_or_qualifier(pars))
            return false;
    }
    LEAVE("parse_specifier_qualifier_list");
    return true;
}


static bool parse_parameter_type_list(PARSER *pars);

/*
abstract_declarator
    = pointer
    | [pointer] '(' abstract_declarator ')'
                { '[' [constant_expression] ']'
                    | '(' [parameter_type_list] ')' }
*/
static bool parse_abstract_declarator(PARSER *pars)
{
    ENTER("parse_abstract_declarator");
    assert(pars);

    if (is_token(pars, TK_STAR)) {
        if (!parse_pointer(pars))
            return false;
        if (!is_token(pars, TK_LPAR))
            goto done;
    } 
    if (!expect(pars, TK_LPAR))
        return false;
    if (!parse_abstract_declarator(pars))
        return false;
    if (!expect(pars, TK_RPAR))
        return false;
    for (;;) {
        if (is_token(pars, TK_LBRA)) {
            next(pars);
            if (is_constant_expression(pars)) {
                if (!parse_constant_expression(pars))
                    return false;
            }
            if (!expect(pars,TK_RBRA))
                return false;
        } else if (is_token(pars, TK_LPAR)) {
            next(pars);
            if (is_parameter_type_list(pars)) {
                if (!parse_parameter_type_list(pars))
                    return false;
            }
            if (!expect(pars, TK_RPAR))
                return false;
        } else
            break;
    }
done:
    LEAVE("parse_abstract_declarator");
    return true;
}

/*
type_name
	= specifier_qualifier_list [abstract_declarator]
*/
static bool parse_type_name(PARSER *pars)
{
    ENTER("parse_type_name");
    assert(pars);

    if (!parse_specifier_qualifier_list(pars))
        return false;
    if (is_abstract_declarator(pars)) {
        if (!parse_abstract_declarator(pars))
            return false;
    }
    LEAVE("parse_type_name");
    return true;
}


/*
parameter_abstract_declarator
    = [pointer] [IDENTIFIER | '(' parameter_abstract_declarator ')']
                { '[' [constant_expression] ']'
                    | '(' [parameter_type_list] ')'
                    | '(' identifier_list ')' }}
*/
static bool parse_parameter_abstract_declarator(PARSER *pars)
{
    ENTER("parse_parameter_abstract_declarator");

    assert(pars);

    if (is_token(pars, TK_STAR)) {
        if (!parse_pointer(pars))
            return false;
    }
    if (is_token(pars, TK_ID)) {
        next(pars);
    } else if (is_token(pars, TK_LPAR)) {
        next(pars);
        if (!parse_parameter_abstract_declarator(pars))
            return false;
        if (!expect(pars, TK_RPAR))
            return false;
    }
    for (;;) {
        if (is_token(pars, TK_LBRA)) {
            next(pars);
            if (is_constant_expression(pars)) {
                if (!parse_constant_expression(pars))
                    return false;
            }
            if (!expect(pars,TK_RBRA))
                return false;
        } else if (is_token(pars, TK_LPAR)) {
            next(pars);
            if (is_parameter_type_list(pars)) {
                if (!parse_parameter_type_list(pars))
                    return false;
            } else if (is_token(pars, TK_ID)) {
                if (!parse_identifier_list(pars))
                    return false;
            }
            if (!expect(pars, TK_RPAR))
                return false;
        } else
            break;
    }

    LEAVE("parse_parameter_abstract_declarator");
    return true;
}

/*
parameter_declaration
	= declaration_specifiers parameter_abstract_declarator
*/
static bool parse_parameter_declaration(PARSER *pars)
{
    TYPE *typ = new_unknown_type();
    STORAGE_CLASS sc = SC_DEFAULT;
    TYPE_QUALIFIER tq = TQ_DEFAULT;

    ENTER("parse_parameter_declaration");
    assert(pars);

    if (!parse_declaration_specifiers(pars, false, true, &typ, &sc, &tq))
        return false;
    if (!parse_parameter_abstract_declarator(pars))
        return false;
    LEAVE("parse_parameter_declaration");
    return true;
}

/*
parameter_list
	= parameter_declaration {',' parameter_declaration}
*/
static bool parse_parameter_list(PARSER *pars)
{
    ENTER("parse_parameter_list");

    assert(pars);

    if (!parse_parameter_declaration(pars))
        return false;
    while (is_token(pars, TK_COMMA)) {
        next(pars);
        if (!parse_parameter_declaration(pars))
            return false;
    }
    LEAVE("parse_parameter_list");
    return true;
}

/*
parameter_type_list
    = parameter_list [',' '...']
*/
static bool parse_parameter_type_list(PARSER *pars)
{
    ENTER("parse_parameter_type_list");
    assert(pars);

    if (!parse_parameter_list(pars))
        return false;
    if (is_token(pars, TK_COMMA)) {
        next(pars);
        if (!expect(pars, TK_ELLIPSIS))
            return false;
    }

    LEAVE("parse_parameter_type_list");
    return true;
}

/*
declarator
	= [pointer] (IDENTIFIER | '(' declarator ')')
                { '[' [constant_expression] ']'
                    | '(' parameter_type_list ')'
                    | '(' [identifier_list] ')' }
*/
static bool parse_declarator(PARSER *pars)
{
    ENTER("parse_declarator");

    assert(pars);

    if (is_token(pars, TK_STAR)) {
        if (!parse_pointer(pars))
            return false;
    }
    if (is_token(pars, TK_ID)) {
        next(pars);
    } else if (is_token(pars, TK_LPAR)) {
        next(pars);
        if (!parse_declarator(pars))
            return false;
        if (!expect(pars, TK_RPAR))
            return false;
    } else {
        parser_error(pars, "need IDENTIFIER or '('");
        return false;
    }

    for (;;) {
        if (is_token(pars, TK_LBRA)) {
            next(pars);
            if (is_constant_expression(pars)) {
                if (!parse_constant_expression(pars))
                    return false;
            }
            if (!expect(pars,TK_RBRA))
                return false;
        } else if (is_token(pars, TK_LPAR)) {
            next(pars);
            if (is_parameter_type_list(pars)) {
                if (!parse_parameter_type_list(pars))
                    return false;
            } else if (is_token(pars, TK_ID)) {
                if (!parse_identifier_list(pars))
                    return false;
            }
            if (!expect(pars, TK_RPAR))
                return false;
        } else
            break;
    }
    LEAVE("parse_declarator");
    return true;
}

/*
struct_declarator
	= declarator
	| [declarator] ':' constant_expression
*/
static bool parse_struct_declarator(PARSER *pars)
{
    ENTER("parse_struct_declarator");
    assert(pars);

    if (is_token(pars, TK_COLON)) {
        next(pars);
        if (!parse_constant_expression(pars))
            return false;
    } else {
        if (!parse_declarator(pars))
            return false;
        if (is_token(pars, TK_COLON)) {
            next(pars);
            if (!parse_constant_expression(pars))
                return false;
        }
    }
    LEAVE("parse_struct_declarator");
    return true;
}

/*
struct_declarator_list
    = struct_declarator {',' struct_declarator}
*/
static bool parse_struct_declarator_list(PARSER *pars)
{
    ENTER("parse_struct_declarator_list");
    assert(pars);

    if (!parse_struct_declarator(pars))
        return false;
    while (is_token(pars, TK_COMMA)) {
        next(pars);
        if (!parse_struct_declarator(pars))
            return false;
    }
    LEAVE("parse_struct_declarator_list");
    return true;
}

/*
struct_declaration
	= specifier_qualifier_list struct_declarator_list ';'
*/
static bool parse_struct_declaration(PARSER *pars)
{
    ENTER("parse_struct_declaration");
    assert(pars);

    if (!parse_specifier_qualifier_list(pars))
        return false;
    if (!parse_struct_declarator_list(pars))
        return false;
    if (!expect(pars, TK_SEMI))
        return false;
    LEAVE("parse_struct_declaration");
    return true;
}

/*
struct_declaration_list
    = struct_declaration {struct_declaration}
*/
static bool parse_struct_declaration_list(PARSER *pars)
{
    ENTER("parse_struct_declaration_list");
    assert(pars);

    if (!parse_struct_declaration(pars))
        return false;
    while (is_struct_declaration(pars)) {
        if (!parse_struct_declaration(pars))
            return false;
    }
    LEAVE("parse_struct_declaration_list");
    return true;
}

/*
struct_or_union_specifier
	= struct_or_union [IDENTIFIER] '{' struct_declaration_list '}'
	= struct_or_union IDENTIFIER
struct_or_union
	= STRUCT | UNION
*/
static bool parse_struct_or_union_specifier(PARSER *pars)
{
    ENTER("parse_struct_or_union_specifier");
    assert(pars);

    next(pars); /* skip TK_STRUCT or TK_UNION */
    if (!is_token(pars, TK_BEGIN)) {
        if (!expect(pars, TK_ID))
            return false;
    }
    if (is_token(pars, TK_BEGIN)) {
        next(pars);
        if (!parse_struct_declaration_list(pars))
            return false;
        if (!expect(pars, TK_END))
            return false;
    }

    LEAVE("parse_struct_or_union_specifier");
    return true;
}

/*
enumerator
    = IDENTIFIER ['=' constant_expression]
*/
static bool parse_enumerator(PARSER *pars)
{
    ENTER("parse_enumerator");
    assert(pars);

    if (!expect_id(pars))
        return false;
    next(pars);
    if (is_token(pars, TK_ASSIGN)) {
        next(pars);
        if (!parse_constant_expression(pars))
            return false;
    }
    LEAVE("parse_enumerator");
    return true;
}

/*
enumerator_list
    = enumerator {',' enumerator}
*/
static bool parse_enumerator_list(PARSER *pars)
{
    ENTER("parse_enumerator_list");
    assert(pars);

    if (!parse_enumerator(pars))
        return false;
    while (is_token(pars, TK_COMMA)) {
        next(pars);
        if (!parse_enumerator(pars))
            return false;
    }
    LEAVE("parse_enumerator_list");
    return true;
}

/*
enum_specifier
	= ENUM [IDENTIFIER] '{' enumerator_list '}'
	| ENUM IDENTIFIER
*/
static bool parse_enum_specifier(PARSER *pars)
{
    ENTER("parse_enum_specifier");
    assert(pars);

    next(pars); /* skip TK_ENUM */
    if (!is_token(pars, TK_BEGIN)) {
        if (!expect_id(pars))
            return false;
        next(pars);
    }

    if (is_token(pars, TK_BEGIN)) {
        next(pars);
        if (!parse_enumerator_list(pars))
            return false;
        if (!expect(pars, TK_END))
            return false;
    }

    LEAVE("parse_enum_specifier");
    return true;
}

/*
declaration_specifier
	= storage_class_specifier | type_specifier | type_qualifier
storage_class_specifier
    = AUTO | REGISTER | STATIC | EXTERN | TYPEDEF
type_specifier
	= VOID | CHAR | SHORT | INT | LONG | FLOAT | DOUBLE
	| SIGNED | UNSIGNED | struct_or_union_specifier
	| enum_specifier | typedef_name
type_qualifier
	= CONST | VOLATILE
*/
static bool parse_declaration_specifier(PARSER *pars,
                bool file_scoped, bool parametered,
                TYPE *typ, STORAGE_CLASS *psc, TYPE_QUALIFIER *ptq)
{
    bool combine_error = false;
    ENTER("parse_declaration_specifier");

    assert(pars);

    switch (pars->token) {
    case TK_AUTO:
        next(pars);
        if (file_scoped)
            goto illegal_sc_on_file_scoped;
        if (parametered)
            goto invalid_in_func_decl;
        if (*psc == SC_AUTO)
            goto dup_warning;
        if (*psc != SC_DEFAULT)
            goto cannot_combine_decl_spec;
        *psc = SC_AUTO;
        break;
    case TK_REGISTER:
        next(pars);
        if (file_scoped)
            goto illegal_sc_on_file_scoped;
        if (*psc == SC_REGISTER)
            goto dup_warning;
        if (*psc != SC_DEFAULT)
            goto cannot_combine_decl_spec;
        *psc = SC_REGISTER;
        break;
    case TK_STATIC:
        next(pars);
        if (parametered)
            goto invalid_in_func_decl;
        if (*psc == SC_STATIC)
            goto dup_warning;
        if (*psc != SC_DEFAULT)
            goto cannot_combine_decl_spec;
        *psc = SC_STATIC;
        break;
    case TK_EXTERN:
        next(pars);
        if (parametered)
            goto invalid_in_func_decl;
        if (*psc == SC_EXTERN)
            goto dup_warning;
        if (*psc != SC_DEFAULT)
            goto cannot_combine_decl_spec;
        *psc = SC_EXTERN;
        break;
    case TK_TYPEDEF:
        next(pars);
        if (parametered)
            goto invalid_in_func_decl;
        if (*psc == SC_TYPEDEF)
            goto dup_warning;
        if (*psc != SC_DEFAULT)
            goto cannot_combine_decl_spec;
        *psc = SC_TYPEDEF;
        break;
    case TK_VOID:
        next(pars);
        if (typ->kind != T_UNKNOWN)
            goto cannot_combine_decl_spec;
        typ->kind  = T_VOID;
        break;
    case TK_CHAR:
        next(pars);
        if (typ->kind == T_SIGNED)
            typ->kind = T_CHAR;
        else if (typ->kind == T_UNSIGNED || typ->kind == T_UNKNOWN)
            typ->kind = T_UCHAR;
        else
            goto cannot_combine_decl_spec;
        break;
    case TK_SHORT:
        next(pars);
        if (typ->kind == T_SIGNED || typ->kind == T_UNKNOWN)
            typ->kind = T_SHORT;
        else if (typ->kind == T_UNSIGNED)
            typ->kind = T_USHORT;
        else
            goto cannot_combine_decl_spec;
        break;
    case TK_INT:
        next(pars);
        if (typ->kind == T_SIGNED || typ->kind == T_UNKNOWN)
            typ->kind = T_INT;
        else if (typ->kind == T_UNSIGNED)
            typ->kind = T_UINT;
        else
            goto cannot_combine_decl_spec;
        break;
    case TK_LONG:
        next(pars);
        if (typ->kind == T_SIGNED || typ->kind == T_UNKNOWN)
            typ->kind = T_LONG;
        else if (typ->kind == T_UNSIGNED)
            typ->kind = T_ULONG;
        else
            goto cannot_combine_decl_spec;
        break;
    case TK_FLOAT:
        next(pars);
        if (typ->kind != T_UNKNOWN)
            goto cannot_combine_decl_spec;
        typ->kind = T_FLOAT;
        break;
    case TK_DOUBLE:
        next(pars);
        if (typ->kind != T_UNKNOWN)
            goto cannot_combine_decl_spec;
        typ->kind = T_DOUBLE;
        break;
    case TK_SIGNED:
        next(pars);
        switch (typ->kind) {
        case T_UNKNOWN:
            typ->kind = T_SIGNED;
            break;
        case T_CHAR:
            typ->kind = T_CHAR;
            break;
        case T_SHORT:
            typ->kind = T_SHORT;
            break;
        case T_INT:
            typ->kind = T_INT;
            break;
        case T_LONG:
            typ->kind = T_LONG;
            break;
        default:
            goto cannot_combine_decl_spec;
        }
        break;
    case TK_UNSIGNED:
        next(pars);
        switch (typ->kind) {
        case T_UNKNOWN:
            typ->kind = T_UNSIGNED;
            break;
        case T_CHAR:
            typ->kind = T_UCHAR;
            break;
        case T_SHORT:
            typ->kind = T_USHORT;
            break;
        case T_INT:
            typ->kind = T_UINT;
            break;
        case T_LONG:
            typ->kind = T_ULONG;
            break;
        default:
            goto cannot_combine_decl_spec;
        }
        break;
    case TK_TYPEDEF_NAME:
        next(pars);
        if (typ->kind != T_UNKNOWN)
            goto cannot_combine_decl_spec;
        typ->kind = T_TYPEDEF_NAME;
        /*TODO*/
        break;
    case TK_STRUCT:
    case TK_UNION:
        if (typ->kind != T_UNKNOWN)
            combine_error = true;
        typ->kind = (pars->token == TK_STRUCT) ? T_STRUCT : T_UNION;
        /*TODO*/
        if (!parse_struct_or_union_specifier(pars))
            return false;
        if (combine_error)
            goto cannot_combine_decl_spec;
        break;
    case TK_ENUM:
        if (typ->kind != T_UNKNOWN)
            combine_error = true;
        typ->kind = T_ENUM;
        /*TODO*/
        if (!parse_enum_specifier(pars))
            return false;
        if (combine_error)
            goto cannot_combine_decl_spec;
        break;
    case TK_CONST:
        next(pars);
        if (*ptq != TQ_DEFAULT)
            goto cannot_combine_decl_spec;
        *ptq = TQ_CONST;
        break;
    case TK_VOLATILE:
        next(pars);
        if (*ptq != TQ_DEFAULT)
            goto cannot_combine_decl_spec;
        *ptq = TQ_VOLATILE;
        break;
    default:
        parser_error(pars, "syntax error");
        return false;
    }
    LEAVE("parse_declaration_specifier");
    return true;

dup_warning:
    parser_warning(pars, "duplicate declaration specifier");
    return true;

illegal_sc_on_file_scoped:
    parser_error(pars, "illegal storage class on file-scoped variable");
    return false;
invalid_in_func_decl:
    parser_error(pars,
        "invalid storage class specifier in function declarator");
    return false;
cannot_combine_decl_spec:
    parser_error(pars, "cannot combine declaration specifier");
    return false;
}

/*
declaration_specifiers
    = declaration_specifier {declaration_specifier}
*/
static bool parse_declaration_specifiers(PARSER *pars,
                bool file_scoped, bool parametered,
                TYPE **typ, STORAGE_CLASS *psc, TYPE_QUALIFIER *ptq)
{
    ENTER("parse_declaration_specifiers");

    assert(pars);

    if (!parse_declaration_specifier(pars, file_scoped, parametered,
                *typ, psc, ptq))
        return false;
    
    while (is_declaration_specifier(pars)) {
        if (!parse_declaration_specifier(pars, file_scoped, parametered,
                    *typ, psc, ptq))
            return false;
    }

    LEAVE("parse_declaration_specifiers");
    return true;
}

/*
external_declaration
	= [declaration_specifiers] [declarator ['=' initializer]
            { ',' declarator ['=' initializer]}]
            (';' | {declaration} compound_statement)
*/
static bool parse_external_declaration(PARSER *pars)
{
    TYPE *typ = new_unknown_type();
    STORAGE_CLASS sc = SC_DEFAULT;
    TYPE_QUALIFIER tq = TQ_DEFAULT;

    ENTER("parse_external_declaration");

    assert(pars);

    if (is_declaration_specifiers(pars)) {
        if (!parse_declaration_specifiers(pars, true, false, &typ, &sc, &tq))
            return false;
    }

    printf("TYPE:%s\n", get_type_string(typ));


    for (;;) {
        if (is_declarator(pars)) {
            if (!parse_declarator(pars))
                return false;

            if (is_token(pars, TK_ASSIGN)) {
                next(pars);
                if (!parse_initializer(pars))
                    return false;
            }
            if (!is_token(pars, TK_COMMA))
                break;
            next(pars);
        } else
            break;
    }
    TRACE("parse_external_declaration", "");
    if (is_token(pars, TK_SEMI)) {
        next(pars);
    } else {
        for (;;) {
            if (is_declaration(pars)) {
                if (!parse_declaration(pars, true))
                    return false;
            } else
                break;
        }
        if (!parse_compound_statement(pars))
            return false;
    }
    LEAVE("parse_external_declaration");
    return true;
}

/*
translation_unit
    = {external_declaration}
*/
bool parse(PARSER *pars)
{
    bool result = true;

    ENTER("parse");

    assert(pars);

    next(pars);
    while (!is_token(pars, TK_EOF)) {
        if (!parse_external_declaration(pars))
            result = false;
    }

    LEAVE("parse");
    return result;
}

