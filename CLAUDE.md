# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A small learning sandbox for C linked lists and S-expression parsing. There are three independent, single-file C programs — no build system, no shared headers, no dependencies beyond libc. `conversation.md` is the transcript of the session that produced them (with the sources duplicated in an appendix) and records the reasoning behind the design choices.

## Build and test

Each `.c` file is a standalone program whose `main` is its own test. **The programs do no input or output by design**: the exit status is the only result (0 = pass, 1 = fail).

```sh
cc -Wall -Wextra -o sexp_parse sexp_parse.c && ./sexp_parse; echo $?
```

Stricter check used when the files were written. On this machine AddressSanitizer aborts at startup (`CHECK failed: sanitizer_allocator_primary64.h`) before any program code runs — that is an environment limitation, not a bug in the sources; drop `address` from `-fsanitize` if it happens:

```sh
gcc -std=c99 -pedantic -Wall -Wextra -fsanitize=address,undefined -o /tmp/t sexp_parse.c && /tmp/t; echo $?
```

There is no way to run a single test case; the tests are `ok &= check(...)` lines in `main`. To isolate one, comment out the others, or temporarily change an expected value to confirm a test can fail.

`sexp_tutor.c` emits one expected warning (unused variable `rc`) because its check is commented out — see below.

## Constraints that aren't obvious from the code

- **Must compile as both C99 and C++.** The files get pasted into Python Tutor with either language setting, so every `malloc`/`calloc`/`realloc` result has an explicit cast. Keep the casts and avoid C-only constructs (verify with `g++ -x c++ file.c`).
- **No I/O.** Don't add `printf` or stdin reading; Python Tutor handles stdin poorly and the original request was for programs with no input or output.
- **`sexp_tutor.c` is bound by Python Tutor's limits**: under 5,600 URL-encoded bytes of source (currently ~4,780) and roughly a thousand execution steps. Steps are counted per source line executed, not per comparison, so function calls are costly and multi-condition one-liners are cheap. That is why its whitespace skip is inlined at three sites instead of being a helper — don't refactor it back into a function.
- **The current `main` in `sexp_tutor.c` is the user's deliberate edit**: it parses `((a) b)` rather than `(a (b c) d)`, and the `ok` check and `sexp_free(tree)` are commented out so the run fits the step limit and the finished tree is still on the heap at the last step. It therefore always exits 0. The commented-out check matches the `(a (b c) d)` input, not the active one.

## How the files relate

`words_to_list.c` → `sexp_parse.c` → `sexp_tutor.c` is a progression, not a layered codebase; nothing is shared between them, and the two parsers define different, incompatible `Sexp` structs.

- `words_to_list.c` — whitespace-split a string into a singly linked list, tracking `head` and `tail` node pointers.
- `sexp_parse.c` — the full parser. `Sexp` is a tagged union (symbol, `long` integer, string, cons). Parse functions return 0/-1 and write through an out-parameter; on failure they free whatever they built.
- `sexp_tutor.c` — a cut-down teaching version of the same parser: symbols only, an untagged struct `{sym, car, cdr}` (atom has `sym` set, cons has `sym == NULL`) because Python Tutor draws that more cleanly than a union, a global `bad` flag instead of return codes, and `abort()` on allocation failure.

Conventions shared by both parsers:

- `NULL` is NIL (the empty list). A proper list `(a b c)` is `(a . (b . (c . NIL)))`.
- `parse_list` appends with a pointer-to-pointer `tail` that starts at `&head` and then advances to each new cell's `cdr` slot. In `sexp_parse.c` the dotted-tail case parses directly into `*tail`.
- Nesting recurses on the C stack; long flat lists are handled iteratively (`sexp_parse.c`'s `sexp_free` and `sexp_equal` recurse on `car` and loop on `cdr`).

`sexp_parse.c` lexical rules worth knowing before changing tests: a token is an integer only if it matches `[+-]?digits` (so `-`, `1+`, `3.14` are symbols, and out-of-range integers are an error); a `.` is the dotted-pair marker only when followed by a delimiter (so `.b` is a symbol); strings accept only the `\"` `\\` `\n` `\t` escapes; `sexp_parse` rejects trailing input after the one expression.

## Other files

The large `.mhtml` file is a saved Python Tutor page kept as a reference for the visualization — don't read it into context.
