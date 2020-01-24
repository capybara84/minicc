#include "minicc.h"

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
    assert(pars);
    assert(pars->scan);
    return &pars->scan->pos;
}

char *get_id(PARSER *pars)
{
    assert(pars);
    assert(pars->scan);
    assert(pars->token == TK_ID);
    return pars->scan->id;
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

static bool is_token_with_array(PARSER *pars, TOKEN tokens[], int count)
{
    TOKEN token;
    int i;

    assert(pars);
    token = pars->token;

    for (i = 0; i < count; i++)
        if (token == tokens[i])
            return true;
    return false;
}

static void skip_error_to(PARSER *pars, TOKEN tokens[], int count)
{
    assert(pars);

    while (!is_token_with_array(pars, tokens, count))
        next(pars);
}

static void skip_error_semi(PARSER *pars)
{
    static TOKEN tokens[] = { TK_SEMI, TK_EOF };
    skip_error_to(pars, tokens, COUNT_OF(tokens));
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
    sprintf(buffer2, "%s at token '%s'", buffer, token_name);

    error(&pars->scan->pos, buffer2);
}

static void parser_warning(PARSER *pars, const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    vwarning(&pars->scan->pos, s, ap);
    va_end(ap);
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

static bool parse_expression(PARSER *pars, NODE **node);

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
        if (!parse_expression(pars, NULL))
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
static bool parse_postfix_expression(PARSER *pars, NODE **exp)
{
    NODE *ep;
    POS pos;

    ENTER("parse_postfix_expression");
    assert(pars);
    assert(exp);

    if (!parse_primary_expression(pars, exp))
        return false;
    pos = *get_pos(pars);
    switch (pars->token) {
    case TK_LBRA:
        next(pars);
        if (!parse_expression(pars, &ep))
            return false;
        if (!expect(pars, TK_RBRA))
            return false;
        *exp = new_node1(NK_ARRAY, &pos, ep);
        break;
    case TK_LPAR:
        next(pars);
        if (!is_token(pars, TK_RPAR)) {
            if (!parse_argument_expression_list(pars, ep))
                return false;
        }
        if (!expect(pars, TK_RPAR))
            return false;
        *exp = new_node1(NK_CALL, &pos, ep);
        break;
    case TK_DOT:
        next(pars);
        if (!expect_id(pars))
            return false;
        *exp = new_node_idnode(NK_DOT, &pos, *exp, get_id(pars));
        next(pars);
        break;
    case TK_PTR:
        next(pars);
        if (!expect_id(pars))
            return false;
        *exp = new_node_idnode(NK_PTR, &pos, *exp, get_id(pars));
        next(pars);
        break;
    case TK_INC:
        next(pars);
        *exp = new_node1(NK_POSTINC, &pos, *exp);
        break;
    case TK_DEC:
        next(pars);
        *exp = new_node1(NK_POSTDEC, &pos, *exp);
        break;
    default:
        break;
    }
    LEAVE("parse_postfix_expression");
    return true;
}

static bool parse_type_name(PARSER *pars);
static bool parse_cast_expression(PARSER *pars, NODE **exp);

/*
unary_expression
    = postfix_expression
    | '++' unary_expression
    | '--' unary_expression
    | unary_operator cast_expression
    | SIZEOF unary_expression
    | SIZEOF '(' type_name ')'
*/
static bool parse_unary_expression(PARSER *pars, NODE **exp)
{
    NODE_KIND kind;
    POS pos;

    ENTER("parse_unary_expression");
    assert(pars);
    assert(exp);

    pos = *get_pos(pars);
    if (is_token(pars, TK_INC) || is_token(pars, TK_DEC)) {
        kind = is_token(pars, TK_INC) ? NK_PREINC : NK_PREDEC;
        next(pars);
        if (!parse_unary_expression(pars, exp))
            return false;
        *exp = new_node1(kind, &pos, *exp);
    } else if (is_unary_operator(pars)) {
        kind = token_to_node(pars->token);
        next(pars);
        if (!parse_cast_expression(pars, exp))
            return false;
        *exp = new_node1(kind, &pos, *exp);
    } else if (is_token(pars, TK_SIZEOF)) {
        /*TODO*/
        next(pars);
        if (is_token(pars, TK_LPAR)) {
            next(pars);
            if (!parse_type_name(pars))
                return false;
            if (!expect(pars, TK_RPAR))
                return false;
        } else {
            if (!parse_unary_expression(pars, exp))
                return false;
        }
        *exp = new_node1(NK_SIZEOF, &pos, *exp);
    } else {
        if (!parse_postfix_expression(pars, exp))
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
static bool parse_cast_expression(PARSER *pars, NODE **exp)
{
    NODE *ep;

    ENTER("parse_cast_expression");
    assert(pars);
    assert(exp);

    *exp = NULL;
    while (is_token(pars, TK_LPAR)) {
        POS pos = *get_pos(pars);
        next(pars);
        if (!parse_type_name(pars))
            return false;
        if (!expect(pars, TK_RPAR))
            return false;
        *exp = new_node(NK_CAST, &pos);  /*TODO type*/
    }
    if (!parse_unary_expression(pars, &ep))
        return false;
    if (*exp == NULL)
        *exp = ep;
    else
        (*exp)->u.link.left = ep;   /*TODO */
    LEAVE("parse_cast_expression");
    return true;
}

/*
multiplicative_expression
	= cast_expression {('*'|'/'|'%') cast_expression}
*/
static bool parse_multiplicative_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_multiplicative_expression");
    assert(pars);
    assert(exp);

    if (!parse_cast_expression(pars, exp))
        return false;
    while (is_token(pars, TK_STAR) || is_token(pars, TK_SLASH)
            || is_token(pars, TK_PERCENT)) {
        NODE *rhs;
        POS pos = *get_pos(pars);
        NODE_KIND kind = token_to_node(pars->token);
        next(pars);
        if (!parse_cast_expression(pars, &rhs))
            return false;
        /*TODO type check */
        *exp = new_node2(kind, &pos, *exp, rhs);
    }
    LEAVE("parse_multiplicative_expression");
    return true;
}

/*
additive_expression
	= multiplicative_expression {('+'|'-') multiplicative_expression}
*/
static bool parse_additive_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_additive_expression");
    assert(pars);
    assert(exp);

    if (!parse_multiplicative_expression(pars, exp))
        return false;
    while (is_token(pars, TK_PLUS) || is_token(pars, TK_MINUS)) {
        NODE *rhs;
        POS pos = *get_pos(pars);
        NODE_KIND kind = token_to_node(pars->token);
        next(pars);
        if (!parse_multiplicative_expression(pars, &rhs))
            return false;
        /*TODO type check */
        *exp = new_node2(kind, &pos, *exp, rhs);
    }
    LEAVE("parse_additive_expression");
    return true;
}

/*
shift_expression
	= additive_expression {('<<'|'>>') additive_expression}
*/
static bool parse_shift_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_shift_expression");
    assert(pars);
    assert(exp);

    if (!parse_additive_expression(pars, exp))
        return false;
    while (is_token(pars, TK_LEFT) || is_token(pars, TK_RIGHT)) {
        NODE *rhs;
        POS pos = *get_pos(pars);
        NODE_KIND kind = token_to_node(pars->token);
        next(pars);
        if (!parse_additive_expression(pars, &rhs))
            return false;
        /*TODO type check */
        *exp = new_node2(kind, &pos, *exp, rhs);
    }
    LEAVE("parse_shift_expression");
    return true;
}

/*
relational_expression
	= shift_expression {('<'|'>'|'<='|'>=') shift_expression}
*/
static bool parse_relational_expression(PARSER *pars, NODE **exp)
{
    TOKEN shift_op[] = { TK_LT, TK_GT, TK_LE, TK_GE };
    ENTER("parse_relational_expression");
    assert(pars);
    assert(exp);

    if (!parse_shift_expression(pars, exp))
        return false;
    while (is_token_with_array(pars, shift_op, COUNT_OF(shift_op))) {
        NODE *rhs;
        POS pos = *get_pos(pars);
        NODE_KIND kind = token_to_node(pars->token);
        next(pars);
        if (!parse_shift_expression(pars, &rhs))
            return false;
        /*TODO type check */
        *exp = new_node2(kind, &pos, *exp, rhs);
    }
    LEAVE("parse_relational_expression");
    return true;
}

/*
equality_expression
	= relational_expression {('=='|'!=') relational_expression}
*/
static bool parse_equality_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_equality_expression");
    assert(pars);
    assert(exp);

    if (!parse_relational_expression(pars, exp))
        return false;
    while (is_token(pars, TK_EQ) || is_token(pars, TK_NEQ)) {
        NODE *rhs;
        POS pos = *get_pos(pars);
        NODE_KIND kind = token_to_node(pars->token);
        next(pars);
        if (!parse_relational_expression(pars))
            return false;
        /*TODO type check */
        *exp = new_node2(kind, &pos, *exp, rhs);
    }
    LEAVE("parse_equality_expression");
    return true;
}

/*
and_expression
	= equality_expression {'&' equality_expression}
*/
static bool parse_and_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_and_expression");
    assert(pars);
    assert(exp);

    if (!parse_equality_expression(pars, exp))
        return false;
    while (is_token(pars, TK_AND)) {
        NODE *rhs;
        POS pos = *get_pos(pars);
        next(pars);
        if (!parse_equality_expression(pars, &rhs))
            return false;
        /*TODO type check */
        *exp = new_node2(NK_AND, &pos, *exp, rhs);
    }
    LEAVE("parse_and_expression");
    return true;
}

/*
exclusive_or_expression
	= and_expression {'^' and_expression}
*/
static bool parse_exclusive_or_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_exclusive_or_expression");
    assert(pars);
    assert(exp);

    if (!parse_and_expression(pars, exp))
        return false;
    while (is_token(pars, TK_HAT)) {
        NODE *rhs;
        POS pos = *get_pos(pars);
        next(pars);
        if (!parse_and_expression(pars, &rhs))
            return false;
        /*TODO type check */
        *exp = new_node2(NK_XOR, &pos, *exp, rhs);
    }
    LEAVE("parse_exclusive_or_expression");
    return true;
}

/*
inclusive_or_expression
	= exclusive_or_expression {'|' exclusive_or_expression}
*/
static bool parse_inclusive_or_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_inclusive_or_expression");
    assert(pars);
    assert(exp);

    if (!parse_exclusive_or_expression(pars, exp))
        return false;
    while (is_token(pars, TK_OR)) {
        NODE *rhs;
        POS pos = *get_pos(pars);
        next(pars);
        if (!parse_exclusive_or_expression(pars, &rhs))
            return false;
        /*TODO type check */
        *exp = new_node2(NK_OR, &pos, *exp, rhs);
    }
    LEAVE("parse_inclusive_or_expression");
    return true;
}

/*
logical_and_expression
	= inclusive_or_expression {'&&' inclusive_or_expression}
*/
static bool parse_logical_and_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_logical_and_expression");
    assert(pars);
    assert(exp);

    if (!parse_inclusive_or_expression(pars, exp))
        return false;
    while (is_token(pars, TK_LAND)) {
        NODE *rhs;
        POS pos = *get_pos(pars);
        next(pars);
        if (!parse_inclusive_or_expression(pars, &rhs))
            return false;
        /*TODO type check */
        *exp = new_node2(NK_LAND, &pos, *exp, rhs);
    }
    LEAVE("parse_logical_and_expression");
    return true;
}

/*
logical_or_expression
	= logical_and_expression {'||' logical_and_expression}
*/
static bool parse_logical_or_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_logical_or_expression");
    assert(pars);
    assert(exp);

    if (!parse_logical_and_expression(pars, exp))
        return false;
    while (is_token(pars, TK_LOR)) {
        NODE *rhs;
        POS pos = *get_pos(pars);
        next(pars);
        if (!parse_logical_and_expression(pars, &rhs))
            return false;
        /*TODO type check */
        *exp = new_node2(NK_LOR, &pos, *exp, rhs);
    }
    LEAVE("parse_logical_or_expression");
    return true;
}

/*
conditional_expression
    = logical_or_expression ['?' expression ':' conditional_expression]
*/
static bool parse_conditional_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_conditional_expression");

    assert(pars);
    assert(exp);

    if (!parse_logical_or_expression(pars, exp))
        return false;
    if (is_token(pars, TK_QUES)) {
        NODE *e2, e3;
        POS pos = *get_pos(pars);
        next(pars);
        if (!parse_expression(pars, &e2))
            return false;
        if (!expect(pars, TK_SEMI))
            return false;
        if (!parse_conditional_expression(pars, &e3))
            return false;
        /*TODO type check */
        *exp = new_node2(NK_COND, &pos, exp,
                    new_node2(NK_COND2, &pos, e2, e3));
    }
    LEAVE("parse_conditional_expression");
    return true;
}

/*
assignment_expression
	= conditional_expression
	| unary_expression assignment_operator assignment_expression
*/
static bool parse_assignment_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_assignment_expression");
    assert(pars);
    assert(exp);

    if (!parse_conditional_expression(pars, exp))
        return false;
    if (is_assignment_operator(pars)) {
        NODE *rhs;
        POS pos = *get_pos(pars);
        NODE_KIND kind = token_to_node(pars->token);
        next(pars);
        if (!parse_assignment_expression(pars, &rhs))
            return false;
        /*TODO type check */
        *exp = new_node2(kind, &pos, *exp, rhs);
    }
    LEAVE("parse_assignment_expression");
    return true;
}

/*
expression
	= assignment_expression {',' assignment_expression}
*/
static bool parse_expression(PARSER *pars, NODE **exp)
{
    ENTER("parse_expression");
    assert(pars);
    assert(exp);

    if (!parse_assignment_expression(pars, exp))
        return false;
    while (is_token(pars, TK_COMMA)) {
        NODE *e;
        POS pos = *get_pos(pars);
        next(pars);
        if (!parse_assignment_expression(pars, &e))
            return false;
        *exp = new_node2(NK_EXPR_LINK, &pos, *exp, e);
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
    NODE *e = NULL;

    ENTER("parse_constant_expression");

    assert(pars);

    if (!parse_conditional_expression(pars, &e))
        return false;

    LEAVE("parse_constant_expression");
    return true;
}

static bool parse_compound_statement(PARSER *pars, NODE **node);

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
static bool parse_statement(PARSER *pars, NODE **node)
{
    ENTER("parse_statement");
    assert(pars);
    assert(node);
    
    switch (pars->token) {
    case TK_CASE:
        TRACE("parse_statement", "case");
        {
            /*TODO value */
            NODE *b;
            POS pos = *get_pos(pars);
            next(pars);
            if (!parse_constant_expression(pars))
                goto fail;
            if (!expect(pars, TK_COLON))
                goto fail;
            if (!parse_statement(pars, &b))
                goto fail;
            *node = new_node_case(NK_CASE, &pos, 0, b);
        }
        break;
    case TK_DEFAULT:
        TRACE("parse_statement", "default");
        {
            NODE *b;
            POS pos = *get_pos(pars);

            next(pars);
            if (!expect(pars, TK_COLON))
                goto fail;
            if (!parse_statement(pars, &b))
                goto fail;
            *node = new_node1(NK_DEFAULT, &pos, b);
        }
        break;
    case TK_BEGIN:
        TRACE("parse_statement", "compound");
        {
            SYMTAB *tab = enter_scope();
            if (!parse_compound_statement(pars, node)) {
                static TOKEN tokens[] = { TK_SEMI, TK_END, TK_EOF };
                skip_error_to(pars, tokens, COUNT_OF(tokens));
                leave_scope();
                return false;
            }
            assert(*node && (*node)->kind == NK_COMPOUND);
            (*node)->u.comp.symtab = tab;
            leave_scope();
        }
        break;
    case TK_IF:
        TRACE("parse_statement", "if");
        {
            NODE *c, *t, *e;
            POS pos = *get_pos(pars);
            next(pars);
            if (!expect(pars, TK_LPAR))
                goto fail;
            if (!parse_expression(pars, &c))
                goto fail;
            if (!expect(pars, TK_RPAR))
                goto fail;
            if (!parse_statement(pars, &t))
                goto fail;
            if (is_token(pars, TK_ELSE)) {
                next(pars);
                if (!parse_statement(pars, &e))
                    goto fail;
            } else
                e = NULL;
            *node = new_node2(NK_IF, &pos, c,
                                new_node2(NK_THEN, &t->pos, t, e));
        }
        break;
    case TK_SWITCH:
        TRACE("parse_statement", "switch");
        {
            NODE *e, *b;
            POS pos = *get_pos(pars);
            next(pars);
            if (!expect(pars, TK_LPAR))
                goto fail;
            if (!parse_expression(pars, &e))
                goto fail;
            if (!expect(pars, TK_RPAR))
                goto fail;
            if (!parse_statement(pars, &b))
                goto fail;
            *node = new_node2(NK_SWITCH, &pos, e, b);
        }
        break;
    case TK_WHILE:
        TRACE("parse_statement", "while");
        {
            NODE *c, *b;
            POS pos = *get_pos(pars);
            next(pars);
            if (!expect(pars, TK_LPAR))
                goto fail;
            if (!parse_expression(pars, &c))
                goto fail;
            if (!expect(pars, TK_RPAR))
                goto fail;
            if (!parse_statement(pars, &b))
                goto fail;
            *node = new_node2(NK_WHILE, &pos, c, b);
        }
        break;
    case TK_DO:
        TRACE("parse_statement", "do");
        {
            NODE *b, *c;
            POS pos = *get_pos(pars);
            next(pars);
            if (!parse_statement(pars, &b))
                goto fail;
            if (!expect(pars, TK_WHILE))
                goto fail;
            if (!expect(pars, TK_LPAR))
                goto fail;
            if (!parse_expression(pars, &c))
                goto fail;
            if (!expect(pars, TK_RPAR))
                goto fail;
            if (!expect(pars, TK_SEMI))
                goto fail;
            *node = new_node2(NK_DO, &pos, c, b);
        }
        break;
    case TK_FOR:
        TRACE("parse_statement", "for");
        {
            NODE *e1, *e2, *e3, *b;
            POS pos = *get_pos(pars);
            next(pars);
            if (!expect(pars, TK_LPAR))
                goto fail;
            if (is_expression(pars)) {
                if (!parse_expression(pars, &e1))
                    goto fail;
            } else
                e1 = NULL;
            if (!expect(pars, TK_SEMI))
                goto fail;
            if (is_expression(pars)) {
                if (!parse_expression(pars, &e2))
                    goto fail;
            } else
                e2 = NULL;
            if (!expect(pars, TK_SEMI))
                goto fail;
            if (is_expression(pars)) {
                if (!parse_expression(pars, &e3))
                    goto fail;
            } else
                e3 = NULL;
            if (!expect(pars, TK_RPAR))
                goto fail;
            if (!parse_statement(pars, &b))
                goto fail;
            *node = new_node2(NK_FOR, &pos, e1,
                        new_node2(NK_FOR2, &pos, e2,
                            new_node2(NK_FOR3, &pos, e3, b)));
        }
        break;
    case TK_GOTO:
        TRACE("parse_statement", "goto");
        {
            char *id;
            POS pos = *get_pos(pars);

            next(pars);
            if (!expect_id(pars))
                goto fail;
            id = get_id(pars);
            next(pars);
            if (!expect(pars, TK_SEMI))
                goto fail;
            *node = new_node_id(NK_GOTO, &pos, id);
        }
        break;
    case TK_CONTINUE:
        TRACE("parse_statement", "continue");
        {
            POS pos = *get_pos(pars);
            next(pars);
            if (!expect(pars, TK_SEMI))
                goto fail;
            *node = new_node(NK_CONTINUE, &pos);
        }
        break;
    case TK_BREAK:
        TRACE("parse_statement", "break");
        {
            POS pos = *get_pos(pars);
            next(pars);
            if (!expect(pars, TK_SEMI))
                goto fail;
            *node = new_node(NK_BREAK, &pos);
        }
        break;
    case TK_RETURN:
        TRACE("parse_statement", "return");
        {
            NODE *e;
            POS pos = *get_pos(pars);
            next(pars);
            if (is_expression(pars)) {
                if (!parse_expression(pars, &e))
                    goto fail;
            } else
                e = NULL;
            if (!expect(pars, TK_SEMI))
                goto fail;
            *node = new_node1(NK_RETURN, &pos, e);
        }
        break;
    case TK_ID:
        /*TODO label node */
        if (is_next_colon(pars->scan)) {
            TRACE("parse_statement", "label");
            next(pars);
            if (!expect(pars, TK_COLON))
                goto fail;
            break;
        }
        /*THROUGH*/
    default:
        {
            NODE *e;
            POS pos = *get_pos(pars);
            TRACE("parse_statement", "expression");
            if (!is_token(pars, TK_SEMI)) {
                if (!parse_expression(pars, &e))
                    goto fail;
            } else
                e = NULL;
            if (!expect(pars, TK_SEMI))
                goto fail;
            *node = new_node1(NK_EXPR, &pos, e);
        }
        break;
    }
    LEAVE("parse_statement");
    return true;

fail:
    skip_error_semi(pars);
    return false;
}

static bool parse_declaration(PARSER *pars, bool parametered);

/*
compound_statement
	= '{' {declaration} {statement} '}'
*/
static bool parse_compound_statement(PARSER *pars, NODE **node)
{
    ENTER("parse_compound_statement");

    assert(pars);
    assert(node);

    *node = new_node(NK_COMPOUND, get_pos(pars));

    if (!expect(pars, TK_BEGIN))
        return false;
    while (is_declaration(pars)) {
        if (!parse_declaration(pars, false))
            return false;
    }
    while (is_statement(pars)) {
        NODE *np;
        POS pos = *get_pos(pars);
        if (!parse_statement(pars, &np))
            return false;
        (*node)->u.comp.node = node_link(NK_LINK, &pos, np,
                (*node)->u.comp.node);
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

static bool parse_declarator(PARSER *pars, TYPE **typ, char **id);

/*
init_declarator
	= declarator ['=' initializer]
*/
static bool parse_init_declarator(PARSER *pars)
{
    TYPE *typ;
    char *id = NULL;

    ENTER("parse_init_declarator");

    assert(pars);

    typ = new_type(T_UNKNOWN, NULL);
    if (!parse_declarator(pars, &typ, &id))
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
    TYPE *typ = new_type(T_UNKNOWN, NULL);
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
static bool
parse_type_qualifier_list(PARSER *pars, TYPE *typ, TYPE_QUALIFIER *ptq)
{
    ENTER("parse_type_qualifier_list");
    assert(pars);
    assert(typ);
    assert(ptq);
    while (is_token(pars, TK_CONST) || is_token(pars, TK_VOLATILE)) {
        if (is_token(pars, TK_CONST)) {
            *ptq = TQ_CONST;
            typ->is_const = true;
        } else {
            *ptq = TQ_VOLATILE;
        }
        next(pars);
    }
    LEAVE("parse_type_qualifier_list");
    return true;
}

/*
pointer
	= '*' type_qualifier_list {'*' type_qualifier_list}
*/
static bool parse_pointer(PARSER *pars, TYPE **pptyp)
{
    TYPE_QUALIFIER tq = TQ_DEFAULT;

    ENTER("parse_pointer");

    assert(pars);
    assert(pptyp);
    assert(*pptyp);

    next(pars); /* skip '*' */

    *pptyp = new_type(T_POINTER, *pptyp);

    if (!parse_type_qualifier_list(pars, *pptyp, &tq))
        return false;
    while (is_token(pars, TK_STAR)) {
        next(pars);
        *pptyp = new_type(T_POINTER, *pptyp);
        if (!parse_type_qualifier_list(pars, *pptyp, &tq))
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


static bool parse_parameter_type_list(PARSER *pars, PARAM **pl);

/*
abstract_declarator
    = pointer
    | [pointer] '(' abstract_declarator ')'
                { '[' [constant_expression] ']'
                    | '(' [parameter_type_list] ')' }
*/
static bool parse_abstract_declarator(PARSER *pars, TYPE **pptyp,
                char **id, PARAM **param_list)
{
    TYPE *typ = NULL;
    ENTER("parse_abstract_declarator");

    assert(pars);
    assert(pptyp);
    assert(*pptyp);
    assert(id);
    assert(param_list);

    if (is_token(pars, TK_STAR)) {
        if (!parse_pointer(pars, pptyp))
            return false;
        if (!is_token(pars, TK_LPAR))
            goto done;
    } 
    if (!expect(pars, TK_LPAR))
        return false;
    typ = new_type(T_UNKNOWN, NULL);
    if (!parse_abstract_declarator(pars, &typ, id, param_list))
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
            *pptyp = new_type(T_ARRAY, *pptyp);
            /*TODO size*/
        } else if (is_token(pars, TK_LPAR)) {
            next(pars);
            if (is_parameter_type_list(pars)) {
                if (!parse_parameter_type_list(pars, param_list))
                    return false;
            }
            if (!expect(pars, TK_RPAR))
                return false;
            *pptyp = new_type(T_FUNC, *pptyp);
            (*pptyp)->param = *param_list;
        } else
            break;
        if (typ) {
            TYPE *p = typ;
            while (p && p->type && p->type->kind != T_UNKNOWN)
                p = p->type;
            if (p->type == NULL || p->type->kind != T_UNKNOWN) {
                parser_error(pars, "type syntax error");
            } else {
                p->type = *pptyp;
                *pptyp = typ;
            }
        }
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
    TYPE *typ;
    char *id = NULL;
    PARAM *param_list = NULL;
    ENTER("parse_type_name");
    assert(pars);

    if (!parse_specifier_qualifier_list(pars))
        return false;
    if (is_abstract_declarator(pars)) {
        typ = new_type(T_UNKNOWN, NULL);
        if (!parse_abstract_declarator(pars, &typ, &id, &param_list))
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
static bool parse_parameter_abstract_declarator(PARSER *pars,
                TYPE **pptyp, char **id)
{
    TYPE *typ = NULL;
    PARAM *param_list = NULL;
    ENTER("parse_parameter_abstract_declarator");

    assert(pars);
    assert(pptyp);
    assert(*pptyp);
    assert(id);

    if (is_token(pars, TK_STAR)) {
        if (!parse_pointer(pars, pptyp))
            return false;
    }
    if (is_token(pars, TK_ID)) {
        *id = get_id(pars);
        next(pars);
    } else if (is_token(pars, TK_LPAR)) {
        next(pars);
        typ = new_type(T_UNKNOWN, NULL);
        if (!parse_parameter_abstract_declarator(pars, &typ, id))
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
            *pptyp = new_type(T_ARRAY, *pptyp);
            /*TODO size*/
        } else if (is_token(pars, TK_LPAR)) {
            next(pars);
            if (is_parameter_type_list(pars)) {
                if (!parse_parameter_type_list(pars, &param_list))
                    return false;
            } else if (is_token(pars, TK_ID)) {
                if (!parse_identifier_list(pars))   /*TODO*/
                    return false;
            }
            if (!expect(pars, TK_RPAR))
                return false;
            *pptyp = new_type(T_FUNC, *pptyp);
            (*pptyp)->param = param_list;
        } else
            break;
        if (typ) {
            TYPE *p = typ;
            while (p && p->type && p->type->kind != T_UNKNOWN)
                p = p->type;
            if (p->type == NULL || p->type->kind != T_UNKNOWN) {
                parser_error(pars, "type syntax error");
            } else {
                p->type = *pptyp;
                *pptyp = typ;
            }
        }
    }

    LEAVE("parse_parameter_abstract_declarator");
    return true;
}

/*
parameter_declaration
	= declaration_specifiers parameter_abstract_declarator
*/
static bool parse_parameter_declaration(PARSER *pars, PARAM **param)
{
    TYPE *typ;
    STORAGE_CLASS sc = SC_DEFAULT;
    TYPE_QUALIFIER tq = TQ_DEFAULT;
    char *id = NULL;

    ENTER("parse_parameter_declaration");

    assert(pars);
    assert(param);

    typ = new_type(T_UNKNOWN, NULL);

    if (!parse_declaration_specifiers(pars, false, true, &typ, &sc, &tq))
        return false;
    if (sc != SC_DEFAULT && sc != SC_REGISTER) {
        parser_error(pars, "invalid storage class for parameter");
    }
    if (!parse_parameter_abstract_declarator(pars, &typ, &id))
        return false;
    *param = new_param(id, typ);
    (*param)->is_register = (sc == SC_REGISTER);
    LEAVE("parse_parameter_declaration");
    return true;
}

/*
parameter_type_list
    = parameter_list [',' '...']
parameter_list
	= parameter_declaration {',' parameter_declaration}
*/
static bool parse_parameter_type_list(PARSER *pars, PARAM **param_list)
{
    PARAM *param;

    ENTER("parse_parameter_type_list");

    assert(pars);
    assert(param_list);

    if (!parse_parameter_declaration(pars, &param))
        return false;
    *param_list = param;
    while (is_token(pars, TK_COMMA)) {
        next(pars);
        if (is_token(pars, TK_ELLIPSIS)) {
            next(pars);
            param = new_param(NULL, NULL);
            add_param(*param_list, param);
            break;
        }
        if (!parse_parameter_declaration(pars, &param))
            return false;
        add_param(*param_list, param);
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
static bool
parse_declarator(PARSER *pars, TYPE **pptyp, char **id)
{
    TYPE *typ = NULL;
    ENTER("parse_declarator");

    assert(pars);
    assert(pptyp);
    assert(*pptyp);
    assert(id);

    if (is_token(pars, TK_STAR)) {
        if (!parse_pointer(pars, pptyp))
            return false;
    }
    if (is_token(pars, TK_ID)) {
        *id = get_id(pars);
        next(pars);
    } else if (is_token(pars, TK_LPAR)) {
        next(pars);
        typ = new_type(T_UNKNOWN, NULL);
        if (!parse_declarator(pars, &typ, id))
            return false;
        if (!expect(pars, TK_RPAR))
            return false;
        /* TODO id, param_list */
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
            *pptyp = new_type(T_ARRAY, *pptyp);
            /*TODO size*/
        } else if (is_token(pars, TK_LPAR)) {
            PARAM *param = NULL;
            next(pars);
            if (is_parameter_type_list(pars)) {
                if (!parse_parameter_type_list(pars, &param))
                    return false;
            } else if (is_token(pars, TK_ID)) {
                /*TODO*/
                if (!parse_identifier_list(pars))
                    return false;
            }
            if (!expect(pars, TK_RPAR))
                return false;
            *pptyp = new_type(T_FUNC, *pptyp);
            (*pptyp)->param = param;
        } else
            break;
        if (typ) {
            TYPE *p = typ;
            while (p && p->type && p->type->kind != T_UNKNOWN)
                p = p->type;
            if (p->type == NULL || p->type->kind != T_UNKNOWN) {
                parser_error(pars, "type syntax error");
            } else {
                p->type = *pptyp;
                *pptyp = typ;
            }
        }
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
    TYPE *typ;
    char *id;

    ENTER("parse_struct_declarator");
    assert(pars);

    if (is_token(pars, TK_COLON)) {
        next(pars);
        if (!parse_constant_expression(pars))
            return false;
    } else {
        typ = new_type(T_UNKNOWN, NULL);
        id = NULL;
        if (!parse_declarator(pars, &typ, &id))
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
        typ->kind = is_token(pars, TK_STRUCT) ? T_STRUCT : T_UNION;
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
        /*
        if (*ptq != TQ_DEFAULT)
            goto cannot_combine_decl_spec;
        */
        *ptq = TQ_CONST;
        typ->is_const = true;
        break;
    case TK_VOLATILE:
        next(pars);
        /*
        if (*ptq != TQ_DEFAULT)
            goto cannot_combine_decl_spec;
        */
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
    return true;
invalid_in_func_decl:
    parser_error(pars,
        "invalid storage class specifier in function declarator");
    return true;
cannot_combine_decl_spec:
    parser_error(pars, "cannot combine declaration specifier");
    return true;
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
    TYPE *typ;
    STORAGE_CLASS sc = SC_DEFAULT;
    TYPE_QUALIFIER tq = TQ_DEFAULT;
    int count = 0;
    SYMBOL *sym = NULL;
    NODE *np = NULL;

    ENTER("parse_external_declaration");

    assert(pars);

    typ = new_type(T_UNKNOWN, NULL);

    if (is_declaration_specifiers(pars)) {
        if (!parse_declaration_specifiers(pars, true, false, &typ, &sc, &tq))
            return false;
        if (typ->kind == T_SIGNED)
            typ->kind = T_INT;
        else if (typ->kind == T_UNSIGNED)
            typ->kind = T_UINT;
    }
    if (typ->kind == T_UNKNOWN) {
        parser_warning(pars, "type specifier missing, defaults to 'int'");
        typ->kind = T_INT;
    }

    for (;;) {
        TYPE *ntyp = dup_type(typ);
        char *id = NULL;

        if (is_declarator(pars)) {
            if (!parse_declarator(pars, &ntyp, &id))
                return false;
            count++;
            sym = lookup_symbol(id);
            if (sym) {
                if (sym->kind == SK_FUNC && ntyp->kind == T_FUNC) {
                    if (!equal_type(sym->type, ntyp))
                        parser_error(pars, "conflicting types for '%s'", id);
                } else if (sym->kind == SK_FUNC)
                    parser_error(pars, "'%s' different kind of symbol", id);
                else
                    parser_error(pars, "'%s' duplicated", id);/*TODO*/
            } else {
                sym = new_symbol((ntyp->kind == T_FUNC) ? SK_FUNC : SK_GLOBAL,
                                    sc, tq, id, ntyp);
            }

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
        if (count == 0)
            parser_warning(pars, "empty declaration");
    } else {
        PARAM *p;

        if (count != 1)
            parser_error(pars, "declaration syntax error");
        assert(sym);
        if (sym->kind != SK_FUNC)
            parser_error(pars, "invalid function syntax");
        for (;;) {
            if (is_declaration(pars)) {
                if (!parse_declaration(pars, true))
                    return false;
            } else
                break;
        }
        /*
        if (sym->has_body) error
        */
        enter_function(sym);
        for (p = sym->type->param; p != NULL; p = p->next) {
            new_symbol(SK_PARAM, sc, tq, p->id, p->type);
            /* calc offset ? */
        }
        if (!parse_compound_statement(pars, &np))
            return false;
        sym->body = np;
        leave_function();
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

