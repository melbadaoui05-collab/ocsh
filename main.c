/*
 * main.c -- boucle REPL (Read - Eval - Print - Loop) d'ocsh
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ocsh.h"

/* Ctrl+C : le shell ne meurt pas ; il saute une ligne et reaffiche le prompt.
 * (Les fils remettent SIGINT par defaut : ils meurent, eux.) */
static void sigint_handler(int sig)
{
    (void)sig;
    const char nl = '\n';
    write(STDOUT_FILENO, &nl, 1);
}

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

    last_status = run_pipeline(&p);
    free_pipeline(&p);
}

int main(void)
{
    /* SIGINT sans SA_RESTART : getline() est interrompu et on reaffiche le prompt */
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    signal(SIGQUIT, SIG_IGN);
    jobs_init();

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
