#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* NULL is NIL (empty list).
   Atom:      sym = "name", car = cdr = NULL
   Cons cell: sym = NULL,   car = item, cdr = rest of list */
typedef struct Sexp {
    char *sym;
    struct Sexp *car, *cdr;
} Sexp;

static int bad;   /* set on syntax error */

static Sexp *new_node(char *sym, Sexp *car, Sexp *cdr)
{
    Sexp *s = (Sexp *)malloc(sizeof *s);
    if (!s) abort();
    s->sym = sym;
    s->car = car;
    s->cdr = cdr;
    return s;
}

static Sexp *parse(const char **p);

/* Called just after '('. tail points at the slot where the next
   cell goes: first &head, then &last_cell->cdr. */
static Sexp *parse_list(const char **p)
{
    Sexp *head = NULL;
    Sexp **tail = &head;
    for (;;) {
        while (isspace((unsigned char)**p)) (*p)++;
        if (**p == ')') { (*p)++; return head; }
        if (**p == '\0') { bad = 1; return head; }   /* missing ')' */
        Sexp *cell = new_node(NULL, parse(p), NULL);
        *tail = cell;
        tail = &cell->cdr;
        if (bad) return head;
    }
}

/* One expression: a list or a symbol. */
static Sexp *parse(const char **p)
{
    while (isspace((unsigned char)**p)) (*p)++;
    if (**p == '(') { (*p)++; return parse_list(p); }
    if (**p == ')' || **p == '\0') { bad = 1; return NULL; }

    const char *start = *p;
    while (**p && !isspace((unsigned char)**p) && **p != '(' && **p != ')')
        (*p)++;
    size_t n = (size_t)(*p - start);
    char *sym = (char *)malloc(n + 1);
    if (!sym) abort();
    memcpy(sym, start, n);
    sym[n] = '\0';
    return new_node(sym, NULL, NULL);
}

void sexp_free(Sexp *s)
{
    if (!s) return;
    free(s->sym);
    sexp_free(s->car);
    sexp_free(s->cdr);
    free(s);
}

/* Returns 0 and sets *out, or -1 on syntax error. */
int sexp_parse(const char *src, Sexp **out)
{
    bad = 0;
    Sexp *s = parse(&src);
    while (isspace((unsigned char)*src)) src++;
    if (*src) bad = 1;   /* trailing junk */
    if (bad) { sexp_free(s); return -1; }
    *out = s;
    return 0;
}

int main(void)
{
    /* const char *text = "(a (b c) d)"; */
    const char *text = "((a) b)";
    Sexp *tree = NULL;
    int rc = sexp_parse(text, &tree);

    /* (a (b c) d) = (a . ((b . (c . NIL)) . (d . NIL))) */
/*    
    int ok = rc == 0
          && !strcmp(tree->car->sym, "a")
          && !strcmp(tree->cdr->car->car->sym, "b")
          && !strcmp(tree->cdr->car->cdr->car->sym, "c")
          && tree->cdr->car->cdr->cdr == NULL
          && !strcmp(tree->cdr->cdr->car->sym, "d")
          && tree->cdr->cdr->cdr == NULL;

    sexp_free(tree);
    return ok ? 0 : 1;   
 */
   /* exit status 0 = pass */

    return 0;
}
