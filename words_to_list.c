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
