/*
 * ocsh.h -- declarations communes du mini-shell ocsh
 * Projet 3 -- Systemes d'Exploitation, UIR/ESIN
 */
#ifndef OCSH_H
#define OCSH_H

#include <sys/types.h>

#define MAX_TOKENS 256
#define MAX_ARGS   128
#define MAX_CMDS    32

/* ------------------------------------------------------------------ */
/* Lexer : la ligne est decoupee en "tokens"                           */
/* ------------------------------------------------------------------ */
typedef enum {
    T_WORD,    /* un mot (nom de commande, argument, nom de fichier)   */
    T_PIPE,    /* ->  : notre operateur de pipe                        */
    T_IN,      /* <   : redirection de l'entree                        */
    T_OUT,     /* >   : redirection de la sortie (ecrase)              */
    T_APPEND,  /* >>  : redirection de la sortie (ajoute)              */
    T_BG       /* &   : execution en arriere-plan                      */
} TokType;

typedef struct {
    TokType type;
    char   *text;   /* alloue (malloc) pour T_WORD, NULL sinon */
} Token;

/* ------------------------------------------------------------------ */
/* Parser : les tokens deviennent un "pipeline" de commandes           */
/* ------------------------------------------------------------------ */
typedef struct {
    char *argv[MAX_ARGS];  /* argv[argc] == NULL, pret pour execvp     */
    int   argc;
    char *in_file;         /* < fichier  (NULL si absent)              */
    char *out_file;        /* > ou >> fichier (NULL si absent)         */
    int   append;          /* 1 si >>                                  */
} Command;

typedef struct {
    Command cmds[MAX_CMDS];
    int     ncmds;
    int     background;    /* 1 si la ligne finit par &                */
} Pipeline;

/* lexer.c */
int  lex(const char *line, Token *toks, int max);   /* nb tokens, -1 si erreur */
void free_tokens(Token *toks, int n);

/* parser.c */
int  parse(Token *toks, int n, Pipeline *p);        /* 0 ok, 1 vide, -1 erreur */
void free_pipeline(Pipeline *p);

/* builtins.c */
typedef int (*builtin_fn)(int argc, char **argv);
builtin_fn  find_builtin(const char *name);         /* NULL si pas un builtin */
const char *resolve_alias(const char *name);        /* "list" -> "ls", etc.   */

/* history.c */
void history_add(const char *line);
void history_print(void);

/* jobs.c : suivi des processus en arriere-plan (SIGCHLD) */
void jobs_init(void);
void jobs_add(pid_t pid);
void jobs_report(void);

/* exec.c */
extern int last_status;                             /* code de retour ($?) */
int run_pipeline(Pipeline *p);

/* main.c */
void eval_line(const char *line);

#endif /* OCSH_H */
