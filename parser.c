/*
 * parser.c -- tokens -> pipeline
 *
 * Grammaire :
 *   ligne    := commande ( "->" commande )* [ "&" ]
 *   commande := mot+ ( "<" fichier | ">" fichier | ">>" fichier )*
 *
 * Le parser "vole" les chaines des tokens (text mis a NULL) : c'est
 * free_pipeline() qui les libere ensuite.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ocsh.h"

void free_pipeline(Pipeline *p)
{
    for (int i = 0; i < p->ncmds; i++) {
        Command *c = &p->cmds[i];
        for (int j = 0; j < c->argc; j++) free(c->argv[j]);
        free(c->in_file);
        free(c->out_file);
    }
    memset(p, 0, sizeof *p);
}

static int syntax_error(Token *toks, int n, Pipeline *p, const char *msg)
{
    fprintf(stderr, "ocsh: erreur de syntaxe : %s\n", msg);
    (void)toks; (void)n;
    free_pipeline(p);
    return -1;
}

int parse(Token *t, int n, Pipeline *p)
{
    memset(p, 0, sizeof *p);
    if (n == 0) return 1;                      /* ligne vide */

    Command *c = &p->cmds[0];
    p->ncmds = 1;

    for (int i = 0; i < n; i++) {
        switch (t[i].type) {

        case T_WORD:
            if (c->argc >= MAX_ARGS - 1)
                return syntax_error(t, n, p, "trop d'arguments");
            c->argv[c->argc++] = t[i].text;    /* on prend possession */
            t[i].text = NULL;
            break;

        case T_IN:
        case T_OUT:
        case T_APPEND:
            if (i + 1 >= n || t[i + 1].type != T_WORD)
                return syntax_error(t, n, p, "nom de fichier attendu apres une redirection");
            if (t[i].type == T_IN) {
                free(c->in_file);
                c->in_file = t[i + 1].text;
            } else {
                free(c->out_file);
                c->out_file = t[i + 1].text;
                c->append = (t[i].type == T_APPEND);
            }
            t[i + 1].text = NULL;
            i++;                               /* on saute le nom de fichier */
            break;

        case T_PIPE:
            if (c->argc == 0)
                return syntax_error(t, n, p, "commande manquante avant ->");
            if (p->ncmds >= MAX_CMDS)
                return syntax_error(t, n, p, "trop de commandes dans le pipeline");
            c = &p->cmds[p->ncmds++];
            break;

        case T_BG:
            if (i != n - 1)
                return syntax_error(t, n, p, "& doit etre en fin de ligne");
            p->background = 1;
            break;
        }
    }

    if (c->argc == 0)
        return syntax_error(t, n, p, "commande manquante");

    return 0;
}
