/*
 * history.c -- historique des commandes (builtin "past")
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ocsh.h"

#define HISTORY_MAX 1000

static char *hist[HISTORY_MAX];
static int   hist_n = 0;

void history_add(const char *line)
{
    /* on retire le \n final et on ignore les lignes vides */
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == ' ' || line[len - 1] == '\t'))
        len--;
    size_t start = 0;
    while (start < len && (line[start] == ' ' || line[start] == '\t')) start++;
    if (len == start) return;

    if (hist_n == HISTORY_MAX) {               /* plein : on oublie la plus ancienne */
        free(hist[0]);
        memmove(hist, hist + 1, (HISTORY_MAX - 1) * sizeof *hist);
        hist_n--;
    }
    hist[hist_n++] = strndup(line + start, len - start);
}

void history_print(void)
{
    for (int i = 0; i < hist_n; i++)
        printf("%5d  %s\n", i + 1, hist[i]);
}
