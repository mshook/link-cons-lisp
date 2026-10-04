/* sexp_parse.c — parse an S-expression into a cons-cell tree.
 *
 * Representation: NULL is NIL (the empty list). Every other value is a
 * Sexp node: a symbol, an integer, a string, or a cons pair (car . cdr).
 * A proper list (a b c) is (a . (b . (c . NIL))).
 *
 * Syntax: ( ) .  symbols  integers  "strings" with \" \\ \n \t escapes.
 * Compiles as C99 or C++.
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

typedef enum { S_SYM, S_INT, S_STR, S_CONS } SexpType;

typedef struct Sexp {
    SexpType type;
    union {
        char *sym;                                   /* S_SYM  */
        long  num;                                   /* S_INT  */
        char *str;                                   /* S_STR  */
        struct { struct Sexp *car, *cdr; } cons;     /* S_CONS */
    } u;
} Sexp;

/* ---------- constructors / destructor ---------- */

static Sexp *node(SexpType t)
{
    Sexp *s = (Sexp *)calloc(1, sizeof *s);
    if (s) s->type = t;
    return s;
}

static char *dup_n(const char *s, size_t n)
{
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

Sexp *mk_sym(const char *name)
{
    Sexp *s = node(S_SYM);
    if (s && !(s->u.sym = dup_n(name, strlen(name)))) { free(s); return NULL; }
    return s;
}

Sexp *mk_str(const char *text)
{
    Sexp *s = node(S_STR);
    if (s && !(s->u.str = dup_n(text, strlen(text)))) { free(s); return NULL; }
    return s;
}

Sexp *mk_int(long v)
{
    Sexp *s = node(S_INT);
    if (s) s->u.num = v;
    return s;
}

Sexp *mk_cons(Sexp *car, Sexp *cdr)
{
    Sexp *s = node(S_CONS);
    if (s) { s->u.cons.car = car; s->u.cons.cdr = cdr; }
    return s;
}

/* Recurse on car, iterate along cdr, so long lists don't blow the stack. */
void sexp_free(Sexp *s)
{
    while (s) {
        Sexp *next = NULL;
        switch (s->type) {
        case S_SYM:  free(s->u.sym); break;
        case S_STR:  free(s->u.str); break;
        case S_INT:  break;
        case S_CONS: sexp_free(s->u.cons.car); next = s->u.cons.cdr; break;
        }
        free(s);
        s = next;
    }
}

int sexp_equal(const Sexp *a, const Sexp *b)
{
    while (a && b) {
        if (a->type != b->type) return 0;
        switch (a->type) {
        case S_SYM: return strcmp(a->u.sym, b->u.sym) == 0;
        case S_STR: return strcmp(a->u.str, b->u.str) == 0;
        case S_INT: return a->u.num == b->u.num;
        case S_CONS:
            if (!sexp_equal(a->u.cons.car, b->u.cons.car)) return 0;
            a = a->u.cons.cdr;
            b = b->u.cons.cdr;
            break;
        }
    }
    return a == b;   /* both NULL */
}

/* ---------- parser ---------- */

static void skip_ws(const char **p)
{
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

static int is_delim(char c)
{
    return c == '\0' || isspace((unsigned char)c) || c == '(' || c == ')' || c == '"';
}

static int parse_expr(const char **p, Sexp **out);

/* After the opening quote. */
static int parse_string(const char **p, Sexp **out)
{
    const char *s = *p;
    size_t cap = 16, len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) return -1;

    for (;;) {
        char c = *s++;
        if (c == '\0') { free(buf); return -1; }          /* unterminated */
        if (c == '"') break;
        if (c == '\\') {
            switch (*s++) {
            case '"':  c = '"';  break;
            case '\\': c = '\\'; break;
            case 'n':  c = '\n'; break;
            case 't':  c = '\t'; break;
            default:   free(buf); return -1;              /* bad escape */
            }
        }
        if (len + 1 >= cap) {
            char *nb = (char *)realloc(buf, cap *= 2);
            if (!nb) { free(buf); return -1; }
            buf = nb;
        }
        buf[len++] = c;
    }
    buf[len] = '\0';

    Sexp *n = node(S_STR);
    if (!n) { free(buf); return -1; }
    n->u.str = buf;
    *p = s;
    *out = n;
    return 0;
}

/* A bare token: an integer if it is [+-]?digits, otherwise a symbol. */
static int parse_atom(const char **p, Sexp **out)
{
    const char *start = *p, *e = start;
    while (!is_delim(*e)) e++;
    size_t n = (size_t)(e - start);

    const char *d = start;
    if (*d == '+' || *d == '-') d++;
    int numeric = d < e;
    for (const char *q = d; q < e; q++)
        if (!isdigit((unsigned char)*q)) { numeric = 0; break; }

    Sexp *s;
    if (numeric) {
        char *tok = dup_n(start, n);
        if (!tok) return -1;
        errno = 0;
        long v = strtol(tok, NULL, 10);
        free(tok);
        if (errno == ERANGE) return -1;                   /* overflow */
        s = mk_int(v);
    } else {
        s = node(S_SYM);
        if (s && !(s->u.sym = dup_n(start, n))) { free(s); s = NULL; }
    }
    if (!s) return -1;
    *p = e;
    *out = s;
    return 0;
}

/* After the opening paren. Builds the cdr chain via a pointer to the tail slot. */
static int parse_list(const char **p, Sexp **out)
{
    Sexp *head = NULL;
    Sexp **tail = &head;

    for (;;) {
        skip_ws(p);
        char c = **p;

        if (c == '\0') goto fail;                         /* missing ')' */
        if (c == ')') { (*p)++; *out = head; return 0; }

        if (c == '.' && is_delim((*p)[1])) {              /* dotted tail */
            if (!head) goto fail;                         /* (. x) */
            (*p)++;
            if (parse_expr(p, tail) != 0) goto fail;
            skip_ws(p);
            if (**p != ')') goto fail;                    /* (a . b c) */
            (*p)++;
            *out = head;
            return 0;
        }

        Sexp *item;
        if (parse_expr(p, &item) != 0) goto fail;
        Sexp *cell = mk_cons(item, NULL);
        if (!cell) { sexp_free(item); goto fail; }
        *tail = cell;
        tail = &cell->u.cons.cdr;
    }

fail:
    sexp_free(head);
    return -1;
}

/* On success sets *out (NULL means NIL) and returns 0; on error returns -1
   and leaves *out untouched. */
static int parse_expr(const char **p, Sexp **out)
{
    skip_ws(p);
    char c = **p;

    if (c == '\0' || c == ')') return -1;
    if (c == '(') { (*p)++; return parse_list(p, out); }
    if (c == '"') { (*p)++; return parse_string(p, out); }
    if (c == '.' && is_delim((*p)[1])) return -1;         /* stray dot */
    return parse_atom(p, out);
}

/* Parse exactly one S-expression from src. Returns 0 and sets *out, or -1
   on syntax error / out of memory / trailing input. */
int sexp_parse(const char *src, Sexp **out)
{
    const char *p = src;
    Sexp *s = NULL;
    if (parse_expr(&p, &s) != 0) return -1;
    skip_ws(&p);
    if (*p != '\0') { sexp_free(s); return -1; }
    *out = s;
    return 0;
}

/* ---------- tests: exit status 0 = all pass ---------- */

/* Build a proper list from n items. */
static Sexp *list(int n, ...)
{
    Sexp *items[16];
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; i++) items[i] = va_arg(ap, Sexp *);
    va_end(ap);

    Sexp *r = NULL;
    for (int i = n - 1; i >= 0; i--) r = mk_cons(items[i], r);
    return r;
}

static int check(const char *src, Sexp *expected)
{
    Sexp *got = NULL;
    int ok = sexp_parse(src, &got) == 0 && sexp_equal(got, expected);
    sexp_free(got);
    sexp_free(expected);
    return ok;
}

static int check_error(const char *src)
{
    Sexp *got = NULL;
    if (sexp_parse(src, &got) == 0) { sexp_free(got); return 0; }
    return 1;
}

int main(void)
{
    int ok = 1;

    /* (define (square x) (* x x)) */
    ok &= check("(define (square x) (* x x))",
                list(3, mk_sym("define"),
                        list(2, mk_sym("square"), mk_sym("x")),
                        list(3, mk_sym("*"), mk_sym("x"), mk_sym("x"))));

    ok &= check("  ()  ", NULL);
    ok &= check("((()))", list(1, list(1, NULL)));
    ok &= check("foo", mk_sym("foo"));
    ok &= check("-42", mk_int(-42));

    /* Integers vs symbols that merely look numeric-ish. */
    ok &= check("(+7 - + 1+ 3.14 -0)",
                list(6, mk_int(7), mk_sym("-"), mk_sym("+"),
                        mk_sym("1+"), mk_sym("3.14"), mk_int(0)));

    /* Strings with escapes, adjacent to parens. */
    ok &= check("(\"hi \\\"there\\\"\"\"a\\\\b\\n\")",
                list(2, mk_str("hi \"there\""), mk_str("a\\b\n")));

    /* Dotted pairs. */
    ok &= check("(a . b)", mk_cons(mk_sym("a"), mk_sym("b")));
    ok &= check("(1 2 . 3)", mk_cons(mk_int(1), mk_cons(mk_int(2), mk_int(3))));
    ok &= check("(a . (b c))", list(3, mk_sym("a"), mk_sym("b"), mk_sym("c")));
    ok &= check("(a .b)", list(2, mk_sym("a"), mk_sym(".b")));   /* .b is a symbol */

    /* Errors. */
    ok &= check_error("");
    ok &= check_error("(a b");
    ok &= check_error(")");
    ok &= check_error("a b");             /* trailing input */
    ok &= check_error("(. a)");
    ok &= check_error("(a . )");
    ok &= check_error("(a . b c)");
    ok &= check_error(".");
    ok &= check_error("\"unterminated");
    ok &= check_error("\"bad \\q escape\"");
    ok &= check_error("99999999999999999999999");

    return ok ? 0 : 1;
}
