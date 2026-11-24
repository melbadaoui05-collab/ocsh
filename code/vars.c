/*
 * vars.c -- variables du shell (jalon 4)
 *
 * Deux sortes de variables :
 *   - variables locales du shell : NOM=valeur   (visibles seulement dans ocsh)
 *   - variables exportees        : export NOM   (copiees dans l'environnement,
 *                                  donc heritees par les processus fils)
 *
 * Variables speciales (lecture seule) :
 *   $?  code de retour de la derniere commande
 *   $!  pid de la derniere commande lancee en arriere-plan
 *   $$  pid du shell
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "vars.h"
#include "exec.h"

#define MAX_VARS 256

static struct { char *name; char *value; } vars[MAX_VARS];
static int nvars = 0;

pid_t last_bg_pid = 0;                       /* $! */

/* "NOM=valeur" : NOM = lettre ou _ suivi de lettres, chiffres ou _ */
int is_assignment(const char *w)
{
    size_t i = 0;
    if (!(isalpha((unsigned char)w[0]) || w[0] == '_')) return 0;
    while (isalnum((unsigned char)w[i]) || w[i] == '_') i++;
    return w[i] == '=';
}

static int find_local(const char *name)
{
    for (int i = 0; i < nvars; i++)
        if (strcmp(vars[i].name, name) == 0) return i;
    return -1;
}

/* Valeur d'une variable, ou NULL si elle n'existe pas.
 * Pour $? $! $$ le resultat est dans un tampon statique : a copier tout de suite. */
const char *var_get(const char *name)
{
    static char num[32];

    if (strcmp(name, "?") == 0) { snprintf(num, sizeof num, "%d", last_status); return num; }
    if (strcmp(name, "!") == 0) {
        if (last_bg_pid > 0) snprintf(num, sizeof num, "%d", (int)last_bg_pid);
        else num[0] = '\0';
        return num;
    }
    if (strcmp(name, "$") == 0) { snprintf(num, sizeof num, "%d", (int)getpid()); return num; }

    int i = find_local(name);
    if (i >= 0) return vars[i].value;
    return getenv(name);                     /* variable exportee ou heritee */
}

int var_set(const char *name, const char *value)
{
    if (getenv(name))                        /* deja exportee : on met a jour l'environnement */
        return setenv(name, value, 1);

    int i = find_local(name);
    if (i >= 0) {
        char *copy = strdup(value);
        if (!copy) return -1;
        free(vars[i].value);
        vars[i].value = copy;
        return 0;
    }
    if (nvars >= MAX_VARS) { fprintf(stderr, "ocsh: trop de variables\n"); return -1; }
    char *n = strdup(name), *v = strdup(value);
    if (!n || !v) { free(n); free(v); return -1; }
    vars[nvars].name = n;
    vars[nvars].value = v;
    nvars++;
    return 0;
}

/* export NOM : la variable passe de la table locale a l'environnement */
int var_export(const char *name)
{
    int i = find_local(name);
    if (i >= 0) {
        if (setenv(name, vars[i].value, 1) < 0) return -1;
        free(vars[i].name);
        free(vars[i].value);
        vars[i] = vars[--nvars];             /* on comble le trou avec la derniere */
        return 0;
    }
    if (!getenv(name)) return setenv(name, "", 1);   /* export d'une variable inconnue : vide */
    return 0;
}

/* Si la commande n'est faite que d'assignations (X=1 Y=2), les applique et
 * retourne 1 ; sinon retourne 0 et ne fait rien. */
int vars_try_assign(const Command *c)
{
    if (c->argc == 0) return 0;
    for (int i = 0; i < c->argc; i++)
        if (!is_assignment(c->argv[i])) return 0;

    for (int i = 0; i < c->argc; i++) {
        const char *eq = strchr(c->argv[i], '=');
        char *name = strndup(c->argv[i], (size_t)(eq - c->argv[i]));
        if (!name || var_set(name, eq + 1) < 0)
            fprintf(stderr, "ocsh: assignation impossible : %s\n", c->argv[i]);
        free(name);
    }
    return 1;
}
