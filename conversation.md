# C linked lists and S-expression parsing — conversation

---

**Michael:** Give me a C program to turn a string of words into a linked list of word. No input or output.

**Claude:** I wrote `words_to_list.c`. It splits a string on whitespace and returns a singly linked list of heap-allocated words in order.

- `Node *words_to_list(const char *str)` returns the list head. It returns `NULL` for an empty or all-space string, and also on allocation failure, in which case it frees any partial list.
- `void free_list(Node *head)` frees the nodes and their word strings.

It has no `main` and does no input or output. I haven't compiled or tested it.

---

**Michael:** Add a test string an a main

**Claude:** I added a `main` to `words_to_list.c` with a test string that has leading and trailing spaces, a tab, a newline and repeated spaces. It builds the list, checks all 9 words against the expected ones, frees it, and returns exit status 0 on a pass and 1 on a failure.

I kept it free of input and output, as you asked at the start, so it prints nothing. I compiled it with `-Wall -Wextra` and AddressSanitizer/UBSan, and it ran clean with exit 0.

---

**Michael:** error: invalid conversion from 'void\*' to 'char\*' [-fpermissive] Fix this

**Claude:** You were compiling it as C++ (a `.cpp` file, or `g++`), which won't implicitly convert `void*` from `malloc`. I added explicit casts to both `malloc` calls, in `dup_n` and in `words_to_list`.

It now compiles cleanly with both `gcc` and `g++` (`-Wall -Wextra`), and the test passes with exit 0 in both.

---

**Michael:** Now I would like you to make, in a different file, a C program to parse an S-expression into a hierarchical data structure. Any questions before you start?

**Claude:** *(Asked four multiple-choice questions: list representation, atom types, reader extras, and what `main` should do.)*

**Michael:** 1

**Claude:** I wrote `sexp_parse.c`, which parses one S-expression into a cons-cell tree. All 22 tests pass under both `gcc -std=c99 -pedantic` and `g++`, with AddressSanitizer and UBSan clean. I also changed one expected value to make sure the tests can fail, and the exit code went to 1.

Your answer was just "1", so I took the first option on each question. If you meant something else, say so and I'll change it.

- **Data structure:** `NULL` is NIL (the empty list). Every other value is a `Sexp` node: a symbol, a `long` integer, a string, or a cons pair. So `(a b c)` is stored as `(a . (b . (c . NIL)))`.
- **Functions:** `int sexp_parse(const char *src, Sexp **out)` returns 0 and sets `*out` on success, or -1 on a syntax error, running out of memory, extra input after the expression, or an integer too big for a `long`. `sexp_free` frees a tree, `sexp_equal` compares two trees, and `mk_sym`, `mk_int`, `mk_str` and `mk_cons` build trees by hand.
- **Atoms:** a token made only of `[+-]?digits` is an integer, and anything else is a symbol, so `-`, `1+` and `3.14` are symbols. Strings allow the escapes `\"`, `\\`, `\n` and `\t`; any other escape is an error.
- **Dotted pairs:** `(a . b)` and `(1 2 . 3)` work, and `(a . (b c))` gives the same tree as `(a b c)`. A dot only counts on its own, so `.b` is a symbol. Rejected: `(. a)`, `(a . )`, `(a . b c)`, and a bare `.`.
- **Recursion:** nested lists recurse on the C stack, but long flat lists are built and freed in a loop.
- **Tests:** `main` prints nothing and only sets the exit code, 0 for a pass.

---

**Michael:** Are you familiar with pythontutor.com?

**Claude:** Yes. Python Tutor (pythontutor.com, by Philip Guo) runs your code and shows each step: the stack frames on the left, heap objects on the right, and pointers drawn as arrows between them. It supports Python, Java, JavaScript, C and C++, and runs the C/C++ versions under a Valgrind-based backend. That arrow view suits both programs here.

That may also explain the error you hit earlier: if you pasted the file in with the C++ language setting, it compiles as C++, which rejects `malloc` without a cast.

Practical limits: it stops after a fixed number of execution steps (roughly a thousand for C/C++, from what I recall), so `sexp_parse.c` with its 22 tests won't fit; programs that read stdin aren't well supported; and `calloc` and string literals can clutter the picture. I suggested a cut-down copy with one short test string.

---

**Michael:** *(Uploaded a saved Python Tutor page showing `words_to_list.c` running on `" a bc "`.)* I should say I very much like the way pythontutor.com animated words_to_list.c - However, when I tried to run sexp_parse.c I got this message: Error! your code is around 16960 URL-encoded bytes, which is too long for this tool. Shorten your code to less than 5600 bytes and re-try. [#ErrorCodeTooLong] - Can you do that and still have a example the helps me understand what's going on. … I would like to be able to get a similar display for a simple s expression like (a (b c) d)

**Claude:** I made a cut-down version as a new file, `sexp_tutor.c`, which parses `(a (b c) d)`. It measures about 4,640 URL-encoded bytes, under the 5,600 limit (measured the same way, the original came to 16,911 against the site's 16,960).

What I cut: no union (each node is a plain struct `{ char *sym; Sexp *car, *cdr; }`, which Python Tutor draws cleanly); symbols only; one test string with a single shape check; and `abort()` on `malloc` failure.

What to watch while stepping: `tail` in `parse_list` is a pointer to a pointer that starts at `&head` and then moves to each new pair's `cdr` field; the nested `(b c)` starts a second `parse_list` call, so two `head`/`tail` pairs are on the stack at once; and the finished tree is on the heap just before `sexp_free(tree)` runs.

---

**Michael:** *(Uploaded a second saved page.)* I made a few changes so that it would run to completion in the allowed number of steps

**Claude:** Your version parses `((a) b)` and finishes in 168 steps, and the final heap is the right tree:

```
tree ─► [cons] ──cdr──► [cons] ──cdr──► NIL
          │               │
         car             car
          ▼               ▼
        [cons] ─cdr─► NIL  [atom "b"]
          │
         car
          ▼
        [atom "a"]
```

The first cons cell's `car` is the inner list `(a)`, and the second's `car` is `b`. Commenting out the `ok` check and `sexp_free` leaves the tree on the heap at the last step. I offered to cut steps per character so `(a (b c) d)` fits: inlining the whitespace skip, and reading a symbol with a single test per character.

---

**Michael:** I'm happy for now

**Claude:** All three files are in your outputs: `words_to_list.c`, `sexp_parse.c` (full version) and `sexp_tutor.c` (short version for Python Tutor).

---

**Michael:** What do you mean by, "Reading a symbol with a single test per character."

**Claude:** That phrase was loose, and as advice it was mostly wrong. The symbol loop makes up to four tests per character. I meant replacing them with one check (a helper or a lookup table). But Python Tutor counts steps by source lines executed, not by comparisons, so the four tests on one line already cost one step, and a helper function would add steps. What matters is lines per character: the loop runs two (`while` test, then `(*p)++;`), and writing it on one line might bring that to about one, though I haven't checked how Python Tutor counts a one-line loop. Inlining `skip_ws()` removes call overhead and is probably the bigger saving.

---

**Michael:** Yes, put the whitespace skip inline. Don't do the other.

**Claude:** I removed `skip_ws()` and put its loop directly at the three places that called it. The symbol-reading loop is unchanged. I made the change on your edited version (`((a) b)`, check commented out) and saved it over `sexp_tutor.c`. It's about 4,780 URL-encoded bytes. It compiles and runs with `gcc` and `g++`; the only warning is the now-unused `rc`. I haven't run it on Python Tutor.

---

**Michael:** Output this conversation as markdown

---

## Appendix: final source files

### `words_to_list.c`

```c
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct Node {
    char *word;
    struct Node *next;
} Node;

/* Copy n bytes of s into a new NUL-terminated string. */
static char *dup_n(const char *s, size_t n)
{
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

/* Free a list built by words_to_list. */
void free_list(Node *head)
{
    while (head) {
        Node *next = head->next;
        free(head->word);
        free(head);
        head = next;
    }
}

/* Split str on whitespace; return head of a linked list of words in order.
   Returns NULL for an empty/all-space string or on allocation failure. */
Node *words_to_list(const char *str)
{
    Node *head = NULL, *tail = NULL;
    const char *p = str;

    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;   /* skip spaces */
        if (!*p) break;

        const char *start = p;
        while (*p && !isspace((unsigned char)*p)) p++;  /* scan word */

        Node *n = (Node *)malloc(sizeof *n);
        if (!n) { free_list(head); return NULL; }
        n->word = dup_n(start, (size_t)(p - start));
        n->next = NULL;
        if (!n->word) { free(n); free_list(head); return NULL; }

        if (tail) tail->next = n; else head = n;
        tail = n;
    }
    return head;
}

int main(void)
{
    const char *text = "  the quick  brown\tfox jumps\nover the lazy dog ";
    const char *expected[] = {"the", "quick", "brown", "fox", "jumps",
                              "over", "the", "lazy", "dog"};
    size_t n_expected = sizeof expected / sizeof expected[0];

    Node *head = words_to_list(text);
    size_t i = 0;
    int ok = 1;

    for (Node *n = head; n; n = n->next, i++) {
        if (i >= n_expected || strcmp(n->word, expected[i]) != 0) ok = 0;
    }
    if (i != n_expected) ok = 0;

    free_list(head);
    return ok ? 0 : 1;   /* exit status 0 = pass */
}
```

### `sexp_parse.c`

```c
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
```

### `sexp_tutor.c`

```c
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
```
