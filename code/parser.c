/*
 * parser.c -- analyse d'une ligne de commande
 *
 *   1. lex()   : la ligne devient une liste de tokens (mots, ->, <, >, >>, &)
 *                avec expansion des variables ($VAR, ${VAR}, $?, $!, $$)
 *   2. parse() : les tokens deviennent un Pipeline (liste de Command)
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "vars.h"

/* ======================================================================
 *  PARTIE 1 : LEXER
 *
 *  Regles :
 *   - les espaces separent les mots
 *   - operateurs reconnus meme sans espaces : ->  >>  >  <  &
 *   - '...'  : tout est litteral (aucune expansion)
 *   - "..."  : litteral, sauf \" \\ \$ et les expansions
 *   - \c     : le caractere c est litteral (en dehors des quotes)
 *   - #      : en debut de mot, le reste de la ligne est un commentaire
 * ====================================================================== */
static int is_space(char c) { return c == ' ' || c == '\t' || c == '\n'; }

/* vrai si s commence par un operateur */
static int is_operator(const char *s)
{
    return (s[0] == '-' && s[1] == '>') || s[0] == '>' || s[0] == '<' || s[0] == '&';
}

void free_tokens(Token *toks, int n)
{
    for (int i = 0; i < n; i++) {
        free(toks[i].text);
        toks[i].text = NULL;
    }
}

/* ---------- petit tampon de caracteres qui grandit tout seul ---------- */
/* (l'expansion d'une variable peut rendre un mot plus long que la ligne) */
typedef struct { char *p; size_t len, cap; } SBuf;

static int sb_init(SBuf *sb)
{
    sb->len = 0;
    sb->cap = 64;
    sb->p = malloc(sb->cap);
    if (!sb->p) return -1;
    sb->p[0] = '\0';                         /* mot vide valide : "$INCONNUE" entre guillemets */
    return 0;
}

static int sb_putc(SBuf *sb, char c)
{
    if (sb->len + 2 > sb->cap) {             /* +2 : le caractere et le '\0' final */
        size_t ncap = sb->cap * 2;
        char *np = realloc(sb->p, ncap);
        if (!np) return -1;
        sb->p = np;
        sb->cap = ncap;
    }
    sb->p[sb->len++] = c;
    sb->p[sb->len] = '\0';
    return 0;
}

static int sb_puts(SBuf *sb, const char *s)
{
    for (; *s; s++)
        if (sb_putc(sb, *s) < 0) return -1;
    return 0;
}

/* *sp pointe sur un '$'. Ecrit la valeur dans sb et avance *sp apres
 * l'expression. Retourne 0 si ok, -1 si erreur (message deja affiche). */
static int expand_dollar(const char **sp, SBuf *sb)
{
    const char *s = *sp + 1;                 /* juste apres le '$' */
    char name[256];
    size_t n = 0;

    if (*s == '{') {                         /* ${NOM} */
        s++;
        while (*s && *s != '}' && n < sizeof name - 1) name[n++] = *s++;
        if (*s != '}') {
            fprintf(stderr, "ocsh: ${...} non ferme (ou nom trop long)\n");
            return -1;
        }
        s++;
        name[n] = '\0';
        if (n == 0) {
            fprintf(stderr, "ocsh: ${} : substitution invalide\n");
            return -1;
        }
    } else if (*s == '?' || *s == '!' || *s == '$') {   /* $? $! $$ */
        name[0] = *s++;
        name[1] = '\0';
    } else if (isalpha((unsigned char)*s) || *s == '_') {   /* $NOM */
        while ((isalnum((unsigned char)*s) || *s == '_') && n < sizeof name - 1)
            name[n++] = *s++;
        name[n] = '\0';
    } else {                                 /* '$' seul : on le garde tel quel */
        if (sb_putc(sb, '$') < 0) return -1;
        *sp = s;
        return 0;
    }

    const char *val = var_get(name);         /* NULL -> variable inconnue -> vide */
    if (val && sb_puts(sb, val) < 0) return -1;
    *sp = s;
    return 0;
}

int lex(const char *s, Token *toks, int max)
{
    int n = 0;

    while (*s) {
        while (is_space(*s)) s++;
        if (!*s || *s == '#') break;

        if (n >= max - 1) {
            fprintf(stderr, "ocsh: ligne trop longue (trop de tokens)\n");
            goto fail;
        }

        /* --- operateurs --- */
        if (s[0] == '-' && s[1] == '>') { toks[n].type = T_PIPE;   toks[n++].text = NULL; s += 2; continue; }
        if (s[0] == '>' && s[1] == '>') { toks[n].type = T_APPEND; toks[n++].text = NULL; s += 2; continue; }
        if (s[0] == '>')                { toks[n].type = T_OUT;    toks[n++].text = NULL; s += 1; continue; }
        if (s[0] == '<')                { toks[n].type = T_IN;     toks[n++].text = NULL; s += 1; continue; }
        if (s[0] == '&')                { toks[n].type = T_BG;     toks[n++].text = NULL; s += 1; continue; }

        /* --- mot --- */
        SBuf sb;
        int  had_quote = 0;                  /* le mot contient-il des guillemets ? */
        if (sb_init(&sb) < 0) { perror("ocsh: malloc"); goto fail; }

        while (*s && !is_space(*s) && !is_operator(s)) {
            if (*s == '\'') {                /* '...' : litteral, pas d'expansion */
                had_quote = 1;
                s++;
                while (*s && *s != '\'')
                    if (sb_putc(&sb, *s++) < 0) goto word_oom;
                if (!*s) {
                    fprintf(stderr, "ocsh: guillemet ' non ferme\n");
                    goto word_fail;
                }
                s++;
            } else if (*s == '"') {          /* "..." : litteral + expansions */
                had_quote = 1;
                s++;
                while (*s && *s != '"') {
                    if (*s == '\\' && (s[1] == '"' || s[1] == '\\' || s[1] == '$')) {
                        s++;                 /* \" \\ \$ : on garde le caractere suivant */
                    } else if (*s == '$') {
                        if (expand_dollar(&s, &sb) < 0) goto word_fail;
                        continue;
                    }
                    if (sb_putc(&sb, *s++) < 0) goto word_oom;
                }
                if (!*s) {
                    fprintf(stderr, "ocsh: guillemet \" non ferme\n");
                    goto word_fail;
                }
                s++;
            } else if (*s == '\\' && s[1]) { /* \c */
                if (sb_putc(&sb, s[1]) < 0) goto word_oom;
                s += 2;
            } else if (*s == '$') {          /* expansion hors guillemets */
                if (expand_dollar(&s, &sb) < 0) goto word_fail;
            } else {
                if (sb_putc(&sb, *s++) < 0) goto word_oom;
            }
        }

        if (sb.len == 0 && !had_quote) {     /* variable inconnue : le mot disparait */
            free(sb.p);
            continue;
        }
        toks[n].type = T_WORD;
        toks[n++].text = sb.p;
        continue;

    word_oom:
        perror("ocsh: malloc");
    word_fail:
        free(sb.p);
        goto fail;
    }
    return n;

fail:
    free_tokens(toks, n);
    return -1;
}

/* ======================================================================
 *  PARTIE 2 : PARSER
 *
 *  Grammaire :
 *    ligne    := commande ( "->" commande )* [ "&" ]
 *    commande := mot+ ( "<" fichier | ">" fichier | ">>" fichier )*
 *
 *  Le parser "vole" les chaines des tokens (text mis a NULL) : c'est
 *  free_pipeline() qui les libere ensuite.
 * ====================================================================== */
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
