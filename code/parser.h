#ifndef PARSER_H
#define PARSER_H

#define MAX_TOKENS 256
#define MAX_ARGS   128
#define MAX_CMDS    32

/* Lexer : la ligne est decoupee en "tokens" */
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

/* Parser : les tokens deviennent un "pipeline" de commandes */
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

int  lex(const char *line, Token *toks, int max);   /* nb tokens, -1 si erreur */
void free_tokens(Token *toks, int n);
int  parse(Token *toks, int n, Pipeline *p);        /* 0 ok, 1 vide, -1 erreur */
void free_pipeline(Pipeline *p);

#endif /* PARSER_H */
