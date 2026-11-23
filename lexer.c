/*
 * lexer.c -- decoupage d'une ligne en tokens
 *
 * Regles :
 *   - les espaces separent les mots
 *   - operateurs reconnus meme sans espaces : ->  >>  >  <  &
 *   - '...'  : tout est litteral
 *   - "..."  : litteral, sauf \" et \\
 *   - \c     : le caractere c est litteral (en dehors des quotes)
 *   - #      : en debut de mot, le reste de la ligne est un commentaire
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ocsh.h"

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
        char *buf = malloc(strlen(s) + 1);   /* le mot ne depasse jamais le reste de la ligne */
        if (!buf) { perror("ocsh: malloc"); goto fail; }
        size_t len = 0;

        while (*s && !is_space(*s) && !is_operator(s)) {
            if (*s == '\'') {                /* '...' : litteral */
                s++;
                while (*s && *s != '\'') buf[len++] = *s++;
                if (!*s) {
                    fprintf(stderr, "ocsh: guillemet ' non ferme\n");
                    free(buf);
                    goto fail;
                }
                s++;
            } else if (*s == '"') {          /* "..." : litteral + \" \\ */
                s++;
                while (*s && *s != '"') {
                    if (*s == '\\' && (s[1] == '"' || s[1] == '\\')) s++;
                    buf[len++] = *s++;
                }
                if (!*s) {
                    fprintf(stderr, "ocsh: guillemet \" non ferme\n");
                    free(buf);
                    goto fail;
                }
                s++;
            } else if (*s == '\\' && s[1]) { /* \c */
                buf[len++] = s[1];
                s += 2;
            } else {
                buf[len++] = *s++;
            }
        }
        buf[len] = '\0';
        toks[n].type = T_WORD;
        toks[n++].text = buf;
    }
    return n;

fail:
    free_tokens(toks, n);
    return -1;
}
