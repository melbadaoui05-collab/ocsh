/*
 * main.c -- boucle REPL (Read - Eval - Print - Loop) d'ocsh
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "parser.h"
#include "exec.h"
#include "builtins.h"
#include "vars.h"
#include "signals.h"

/* lit -> decoupe -> analyse -> execute une ligne */
void eval_line(const char *line)
{
    Token    toks[MAX_TOKENS];
    Pipeline p;

    history_add(line);

    int n = lex(line, toks, MAX_TOKENS);
    if (n < 0) { last_status = 2; return; }

    int r = parse(toks, n, &p);
    free_tokens(toks, n);                  /* les mots pris par le parser sont a NULL */
    if (r < 0) { last_status = 2; return; }
    if (r > 0) return;                     /* ligne vide */

    /* ligne faite uniquement d'assignations : X=5 Y=2 */
    if (p.ncmds == 1 && vars_try_assign(&p.cmds[0])) {
        last_status = 0;
        free_pipeline(&p);
        return;
    }

    last_status = run_pipeline(&p);
    free_pipeline(&p);
}

int main(void)
{
    signals_init();

    int    interactive = isatty(STDIN_FILENO);
    char  *line = NULL;
    size_t cap = 0;

    for (;;) {
        jobs_report();                     /* annonce les jobs termines */
        if (interactive) {
            fputs("ocsh> ", stdout);
            fflush(stdout);
        }
        if (getline(&line, &cap, stdin) < 0) {
            if (errno == EINTR) { clearerr(stdin); continue; }   /* Ctrl+C */
            if (interactive) putchar('\n');                      /* Ctrl+D */
            break;
        }
        eval_line(line);
    }
    free(line);
    return last_status;
}
