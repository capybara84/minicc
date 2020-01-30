#include "minicc.h"

/* string buffer / number parse buffer */
#define MAX_BUFFER  256

STRING_POOL g_string_pool = { NULL, NULL };
static int s_string_num = 0;

typedef struct ident {
    struct ident *next;
    char *id;
} IDENT;

static IDENT *s_ident_top = NULL;


STRING *new_string(const char *s)
{
    STRING *p = (STRING*) alloc(sizeof (STRING));
    p->s = str_dup(s);
    p->num = s_string_num++;
    p->next = NULL;
    if (g_string_pool.head == NULL) {
        assert(g_string_pool.tail == NULL);
        g_string_pool.head = g_string_pool.tail = p;
    } else {
        assert(g_string_pool.tail);
        g_string_pool.tail->next = p;
        g_string_pool.tail = p;
    }
    return p;
}

char* new_ident(const char *s)
{
    IDENT *p = (IDENT*) alloc(sizeof (IDENT));
    p->id = str_dup(s);
    p->next = s_ident_top;
    s_ident_top = p;
    return p->id;
}

#ifndef NDEBUG
void print_ident(void)
{
    IDENT *id = s_ident_top;
    printf("ident [\n");
    for (; id != NULL; id = id->next)
        printf(" %p:%s\n", id, id->id);
    printf("]\n");
}
#endif

char *intern(const char *s)
{
    IDENT *id = s_ident_top;

#ifndef NDEBUG
    if (is_debug("ident")) {
        printf("intern(%s)\n", s);
        print_ident();
    }
#endif

    for (; id != NULL; id = id->next)
        if (strcmp(id->id, s) == 0)
            return id->id;
    return new_ident(s);
}

SCANNER *open_scanner_text(const char *filename, const char *text)
{
    SCANNER *s = (SCANNER*) alloc(sizeof (SCANNER));
    if (s == NULL)
        return NULL;
    s->source = text;
    s->size = strlen(text);
    s->current = 0;
    s->ch = ' ';
    s->pos.filename = filename;
    s->pos.line = 1;
    s->num = 0;
    s->id = NULL;
    s->str = NULL;
    return s;
}

SCANNER *open_scanner_file(const char *filename)
{
    FILE *fp;
    size_t size;
    char *s;
    int rd;

    fp = fopen(filename, "r");
    if (fp == NULL)
        return NULL;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    size = ftell(fp);

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    s = (char*) alloc(size + 1);

    rd = fread(s, size, 1, fp);
    if (rd != 1) {
        fclose(fp);
        free(s);
        return NULL;
    }
    s[size-1] = '\0';
    fclose(fp);
    return open_scanner_text(filename, s);
}

bool close_scanner(SCANNER *s)
{
    s_ident_top = NULL;

    if (s == NULL)
        return false;
    free(s);
    return true;
}

static int is_white(int ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'
        || ch == '\f' || ch == '\v';
}

static int is_alpha(int ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

static int is_alnum(int ch)
{
    return is_alpha(ch) || isdigit(ch);
}

static int next_char(SCANNER *scan)
{
    if (scan->ch == '\n')
        scan->pos.line++;
    scan->ch = scan->source[scan->current++];
#ifndef NDEBUG
    if (is_debug("scanner"))
        printf("%s(%d):next_char: '%c'\n",
                scan->pos.filename, scan->pos.line, scan->ch);
#endif
    return scan->ch;
}

static void skip_comment(SCANNER *scan)
{
    next_char(scan);
    for (;;) {
        if (scan->ch == '\0') {
            error(&scan->pos, "unterminated comment");
            return;
        }
        if (scan->ch != '*')
            next_char(scan);
        else if (next_char(scan) == '/') {
            next_char(scan);
            break;
        }
    }
}

static void skip_line(SCANNER *scan)
{
    while (scan->ch != '\0' && scan->ch != '\n') {
        next_char(scan);
    }
}

static TOKEN scan_id(SCANNER *scan)
{
    char buffer[MAX_IDENT+1];
    int i = 0;
    while (is_alnum(scan->ch)) {
        if (i < MAX_IDENT)
            buffer[i++] = scan->ch;
        next_char(scan);
    }
    buffer[i] = '\0';
    if (strcmp(buffer, "sizeof") == 0) return TK_SIZEOF;
    if (strcmp(buffer, "typedef") == 0) return TK_TYPEDEF;
    if (strcmp(buffer, "extern") == 0) return TK_EXTERN;
    if (strcmp(buffer, "static") == 0) return TK_STATIC;
    if (strcmp(buffer, "auto") == 0) return TK_AUTO;
    if (strcmp(buffer, "register") == 0) return TK_REGISTER;
    if (strcmp(buffer, "char") == 0) return TK_CHAR;
    if (strcmp(buffer, "short") == 0) return TK_SHORT;
    if (strcmp(buffer, "int") == 0) return TK_INT;
    if (strcmp(buffer, "long") == 0) return TK_LONG;
    if (strcmp(buffer, "signed") == 0) return TK_SIGNED;
    if (strcmp(buffer, "unsigned") == 0) return TK_UNSIGNED;
    if (strcmp(buffer, "float") == 0) return TK_FLOAT;
    if (strcmp(buffer, "double") == 0) return TK_DOUBLE;
    if (strcmp(buffer, "const") == 0) return TK_CONST;
    if (strcmp(buffer, "volatile") == 0) return TK_VOLATILE;
    if (strcmp(buffer, "void") == 0) return TK_VOID;
    if (strcmp(buffer, "struct") == 0) return TK_STRUCT;
    if (strcmp(buffer, "union") == 0) return TK_UNION;
    if (strcmp(buffer, "enum") == 0) return TK_ENUM;
    if (strcmp(buffer, "if") == 0) return TK_IF;
    if (strcmp(buffer, "else") == 0) return TK_ELSE;
    if (strcmp(buffer, "switch") == 0) return TK_SWITCH;
    if (strcmp(buffer, "case") == 0) return TK_CASE;
    if (strcmp(buffer, "default") == 0) return TK_DEFAULT;
    if (strcmp(buffer, "while") == 0) return TK_WHILE;
    if (strcmp(buffer, "do") == 0) return TK_DO;
    if (strcmp(buffer, "for") == 0) return TK_FOR;
    if (strcmp(buffer, "goto") == 0) return TK_GOTO;
    if (strcmp(buffer, "continue") == 0) return TK_CONTINUE;
    if (strcmp(buffer, "break") == 0) return TK_BREAK;
    if (strcmp(buffer, "return") == 0) return TK_RETURN;
    scan->id = intern(buffer);
/*
    if (is_typedef_name(buffer)) return TK_TYPEDEF_NAME;
*/
    return TK_ID;
}

static TOKEN scan_num(SCANNER *scan)
{
    char buffer[MAX_BUFFER+1];
    int i = 0;
    while (isdigit(scan->ch)) {
        if (i < MAX_BUFFER)
            buffer[i++] = scan->ch;
        next_char(scan);
    }
    buffer[i] = '\0';
    scan->num = atoi(buffer);
    /* TODO impl unsigned, long, float, double */
    return TK_INT_LIT;
}

static unsigned char scan_a_char(SCANNER *scan)
{
    int c = scan->ch;
    if (c == '\\') {
        switch (next_char(scan)) {
        case '0': c = '\0'; break;
        case 'a': c = '\a'; break;
        case 'b': c = '\b'; break;
        case 'f': c = '\f'; break;
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case 'v': c = '\v'; break;
        case '\\': c = '\\'; break;
        case '?': c = '?'; break;
        case '\'': c = '\''; break;
        case '"': c = '"'; break;
        case 'x':
        case 'o':
        default:
            /*TODO impl 0xHH, 0OO*/
            c = scan->ch;
            break;
        }
    }
    next_char(scan);
    return c;
}

static TOKEN scan_char(SCANNER *scan)
{
    next_char(scan);  /* skip ' */
    scan->num = scan_a_char(scan);
    if (scan->ch != '\'')
        error(&scan->pos, "missing ' character");
    else
        next_char(scan);
    return TK_CHAR_LIT;
}

static TOKEN scan_str(SCANNER *scan)
{
    /*TODO string pool */
    static char buffer[MAX_BUFFER+1];
    int i = 0;

    next_char(scan);  /* skip " */

    while (scan->ch != '\"') {
        int c = scan_a_char(scan);
        if (i < MAX_BUFFER)
            buffer[i++] = c;
    }
    buffer[i] = '\0';
    scan->str = new_string(buffer);

    if (scan->ch != '\"')
        error(&scan->pos, "missing \" character");
    else
        next_char(scan);
    return TK_STRING_LIT;
}

bool is_next_colon(SCANNER *scan)
{
    while (is_white(scan->ch))
        next_char(scan);
    return (scan->ch == ':');
}

TOKEN next_token(SCANNER *scan)
{
retry:
    while (is_white(scan->ch))
        next_char(scan);
    if (scan->ch == '\0')
        return TK_EOF;
    if (is_alpha(scan->ch))
        return scan_id(scan);
    if (isdigit(scan->ch))
        return scan_num(scan);
    switch (scan->ch) {
    case '\'':  return scan_char(scan);
    case '\"':  return scan_str(scan);
    case '#':   skip_line(scan);    goto retry;
    case ',':   next_char(scan);    return TK_COMMA;
    case '~':   next_char(scan);    return TK_TILDE;
    case ';':   next_char(scan);    return TK_SEMI;
    case ':':   next_char(scan);    return TK_COLON;
    case '?':   next_char(scan);    return TK_QUES;
    case '(':   next_char(scan);    return TK_LPAR;
    case ')':   next_char(scan);    return TK_RPAR;
    case '[':   next_char(scan);    return TK_LBRA;
    case ']':   next_char(scan);    return TK_RBRA;
    case '{':   next_char(scan);    return TK_BEGIN;
    case '}':   next_char(scan);    return TK_END;
    case '.':
        if (next_char(scan) == '.') {
            if (next_char(scan) == '.') {
                next_char(scan);
                return TK_ELLIPSIS;
            }
            error(&scan->pos, "need '...' (missing '.')");
            next_char(scan);
            goto retry;
        }
        return TK_DOT;
    case '=':
        if (next_char(scan) != '=')
            return TK_ASSIGN;
        next_char(scan);
        return TK_EQ;
    case '!':
        if (next_char(scan) != '=')
            return TK_NOT;
        next_char(scan);
        return TK_NEQ;
    case '*':
        if (next_char(scan) != '=')
            return TK_STAR;
        next_char(scan);
        return TK_MUL_AS;
    case '%':
        if (next_char(scan) != '=')
            return TK_PERCENT;
        next_char(scan);
        return TK_MOD_AS;
    case '^':
        if (next_char(scan) != '=')
            return TK_HAT;
        next_char(scan);
        return TK_XOR_AS;
    case '&':
        if (next_char(scan) == '&') {
            next_char(scan);
            return TK_LAND;
        } else if (scan->ch == '=') {
            next_char(scan);
            return TK_AND_AS;
        }
        return TK_AND;
    case '|':
        if (next_char(scan) == '|') {
            next_char(scan);
            return TK_LOR;
        } else if (scan->ch == '=') {
            next_char(scan);
            return TK_OR_AS;
        }
        return TK_OR;
    case '+':
        if (next_char(scan) == '+') {
            next_char(scan);
            return TK_INC;
        } else if (scan->ch == '=') {
            next_char(scan);
            return TK_ADD_AS;
        }
        return TK_PLUS;
    case '<':
        if (next_char(scan) == '<') {
            if (next_char(scan) == '=') {
                next_char(scan);
                return TK_LEFT_AS;
            }
            return TK_LEFT;
        } else if (scan->ch == '=') {
            next_char(scan);
            return TK_LE;
        }
        return TK_LT;
    case '>':
        if (next_char(scan) == '>') {
            if (next_char(scan) == '=') {
                next_char(scan);
                return TK_RIGHT_AS;
            }
            return TK_RIGHT;
        } else if (scan->ch == '=') {
            next_char(scan);
            return TK_GE;
        }
        return TK_GT;
    case '/':
        if (next_char(scan) == '=') {
            next_char(scan);
            return TK_DIV_AS;
        } else if (scan->ch == '*') {
            skip_comment(scan);
            goto retry;
        }
        return TK_SLASH;
    case '-':
        if (next_char(scan) == '-') {
            next_char(scan);
            return TK_DEC;
        } else if (scan->ch == '=') {
            next_char(scan);
            return TK_SUB_AS;
        } else if (scan->ch == '>') {
            next_char(scan);
            return TK_PTR;
        }
        return TK_MINUS;
    default:
        break;
    }
    error(&scan->pos,
        (isprint(scan->ch) ? "illegal character '%c'"
                          : "illegal character (code=%02d)"), scan->ch);
    next_char(scan);
    goto retry;
}


const char *token_to_string(TOKEN tk)
{
    switch (tk) {
    case TK_EOF:            return "<EOF>";
    case TK_ID:             return "<ID>";
    case TK_CHAR_LIT:       return "<CHAR LIT>";
    case TK_INT_LIT:        return "<INT LIT>";
    case TK_UINT_LIT:       return "<UINT LIT>";
    case TK_LONG_LIT:       return "<LONG LIT>";
    case TK_ULONG_LIT:      return "<ULONG LIT>";
    case TK_FLOAT_LIT:      return "<FLOAT LIT>";
    case TK_DOUBLE_LIT:     return "<DOUBLE LIT>";
    case TK_STRING_LIT:     return "<STRING LIT>";
    case TK_TYPEDEF_NAME:   return "<TYPEDEF NAME>";
    case TK_COMMA:          return ",";
    case TK_TILDE:          return "~";
    case TK_SEMI:           return ";";
    case TK_COLON:          return ":";
    case TK_QUES:           return "?";
    case TK_LPAR:           return "(";
    case TK_RPAR:           return ")";
    case TK_LBRA:           return "[";
    case TK_RBRA:           return "]";
    case TK_BEGIN:          return "{";
    case TK_END:            return "}";
    case TK_DOT:            return ".";
    case TK_ELLIPSIS:       return "...";
    case TK_ASSIGN:         return "=";
    case TK_EQ:             return "==";
    case TK_NOT:            return "!";
    case TK_NEQ:            return "!=";
    case TK_STAR:           return "*";
    case TK_MUL_AS:         return "*=";
    case TK_SLASH:          return "/";
    case TK_DIV_AS:         return "/=";
    case TK_PERCENT:        return "%";
    case TK_MOD_AS:         return "%=";
    case TK_HAT:            return "^";
    case TK_XOR_AS:         return "^=";
    case TK_AND:            return "&";
    case TK_LAND:           return "&&";
    case TK_AND_AS:         return "&=";
    case TK_OR:             return "|";
    case TK_LOR:            return "||";
    case TK_OR_AS:          return "|=";
    case TK_PLUS:           return "+";
    case TK_INC:            return "++";
    case TK_ADD_AS:         return "+=";
    case TK_MINUS:          return "-";
    case TK_DEC:            return "--";
    case TK_SUB_AS:         return "-=";
    case TK_PTR:            return "->";
    case TK_LT:             return "<";
    case TK_LEFT:           return "<<";
    case TK_LEFT_AS:        return "<<=";
    case TK_LE:             return "<=";
    case TK_GT:             return ">";
    case TK_RIGHT:          return ">>";
    case TK_RIGHT_AS:       return ">>=";
    case TK_GE:             return ">=";
    case TK_SIZEOF:         return "sizeof";
    case TK_TYPEDEF:        return "typedef";
    case TK_EXTERN:         return "extern";
    case TK_STATIC:         return "static";
    case TK_AUTO:           return "auto";
    case TK_REGISTER:       return "register";
    case TK_CHAR:           return "char";
    case TK_SHORT:          return "short";
    case TK_INT:            return "int";
    case TK_LONG:           return "long";
    case TK_SIGNED:         return "signed";
    case TK_UNSIGNED:       return "unsigned";
    case TK_FLOAT:          return "float";
    case TK_DOUBLE:         return "double";
    case TK_CONST:          return "const";
    case TK_VOLATILE:       return "volatile";
    case TK_VOID:           return "void";
    case TK_STRUCT:         return "struct";
    case TK_UNION:          return "union";
    case TK_ENUM:           return "enum";
    case TK_IF:             return "if";
    case TK_ELSE:           return "else";
    case TK_SWITCH:         return "switch";
    case TK_CASE:           return "case";
    case TK_DEFAULT:        return "default";
    case TK_WHILE:          return "while";
    case TK_DO:             return "do";
    case TK_FOR:            return "for";
    case TK_GOTO:           return "goto";
    case TK_CONTINUE:       return "continue";
    case TK_BREAK:          return "break";
    case TK_RETURN:         return "return";
    }
    return "";
}

const char *scan_token_to_string(SCANNER *scan, TOKEN tk)
{
    switch (tk) {
    case TK_ID:
        return "<ID>";
    case TK_CHAR_LIT:
        return "<CHAR LIT>";
    case TK_INT_LIT:
        return "<INT LIT>";
    case TK_UINT_LIT:
        return "<UINT LIT>";
    case TK_LONG_LIT:
        return "<LONG LIT>";
    case TK_ULONG_LIT:
        return "<ULONG LIT>";
    case TK_FLOAT_LIT:
        return "<FLOAT LIT>";
    case TK_DOUBLE_LIT:
        return "<DOUBLE LIT>";
    case TK_STRING_LIT:
        return "<STRING LIT>";
    case TK_TYPEDEF_NAME:
        return "<TYPEDEF NAME>";
    default:
        break;
    }
    return token_to_string(tk);
}


