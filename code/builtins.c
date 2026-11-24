/*
 * builtins.c -- commandes integrees (go, quit, path, say, info, past, osinfo, export)
 *
 * Pourquoi "go" (cd) doit etre un builtin : chdir() agit sur le processus
 * qui l'appelle. Dans un fils (fork), le repertoire courant change pour le
 * fils seulement, et le shell ne bouge pas. Meme chose pour "quit" (exit).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "builtins.h"
#include "exec.h"
#include "vars.h"

/* ---------- historique (builtin "past") ---------- */
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


/* ---------- go : cd ---------- */
static int bi_go(int argc, char **argv)
{
    const char *target;
    int print_it = 0;

    if (argc > 2) { fprintf(stderr, "go: trop d'arguments\n"); return 1; }

    if (argc == 1 || strcmp(argv[1], "~") == 0) {
        target = getenv("HOME");
        if (!target) { fprintf(stderr, "go: HOME non defini\n"); return 1; }
    } else if (strcmp(argv[1], "-") == 0) {
        target = getenv("OLDPWD");
        if (!target) { fprintf(stderr, "go: OLDPWD non defini\n"); return 1; }
        print_it = 1;
    } else {
        target = argv[1];
    }

    char old[PATH_MAX];
    if (!getcwd(old, sizeof old)) old[0] = '\0';

    if (chdir(target) < 0) {
        fprintf(stderr, "go: %s: ", target);
        perror(NULL);
        return 1;
    }

    char now[PATH_MAX];
    if (getcwd(now, sizeof now)) setenv("PWD", now, 1);
    if (old[0]) setenv("OLDPWD", old, 1);
    if (print_it) printf("%s\n", now);
    return 0;
}

/* ---------- quit : exit ---------- */
static int bi_quit(int argc, char **argv)
{
    int code = last_status;
    if (argc > 1) code = atoi(argv[1]);
    exit(code);
}

/* ---------- path : pwd ---------- */
static int bi_path(int argc, char **argv)
{
    (void)argc; (void)argv;
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof cwd)) { perror("path"); return 1; }
    printf("%s\n", cwd);
    return 0;
}

/* ---------- say : echo ---------- */
static int bi_say(int argc, char **argv)
{
    int i = 1, newline = 1;
    if (i < argc && strcmp(argv[i], "-n") == 0) { newline = 0; i++; }
    for (; i < argc; i++) {
        fputs(argv[i], stdout);
        if (i < argc - 1) putchar(' ');
    }
    if (newline) putchar('\n');
    return 0;
}

/* ---------- info : help ---------- */
static int bi_info(int argc, char **argv)
{
    (void)argc; (void)argv;
    puts("ocsh -- mini-shell (Systemes d'Exploitation, UIR/ESIN)\n");
    puts("Builtins :");
    puts("  go [dir]     changer de repertoire (cd)   go - : repertoire precedent");
    puts("  quit [code]  quitter le shell (exit)");
    puts("  path         afficher le repertoire courant (pwd)");
    puts("  say [-n] ... afficher des arguments (echo)");
    puts("  info         cette aide (help)");
    puts("  past         historique des commandes (history)");
    puts("  osinfo       informations sur le cours");
    puts("  export [N=v] exporter une variable vers les processus fils\n");
    puts("Variables :");
    puts("  NOM=valeur   definir une variable    $NOM ${NOM}   la lire");
    puts("  $?  code de retour   $!  pid du dernier job &   $$  pid du shell\n");
    puts("Alias :");
    puts("  list=ls  show=cat  copy=cp  move=mv  del=rm  newdir=mkdir  newfile=touch\n");
    puts("Operateurs :");
    puts("  cmd1 -> cmd2   pipe        cmd > f / cmd >> f   sortie vers fichier");
    puts("  cmd < f        entree      cmd &                arriere-plan");
    return 0;
}

/* ---------- past : history ---------- */
static int bi_past(int argc, char **argv)
{
    (void)argc; (void)argv;
    history_print();
    return 0;
}

/* ---------- osinfo ---------- */
static int bi_osinfo(int argc, char **argv)
{
    (void)argc; (void)argv;
    puts("Systèmes d'Exploitation -- UIR/ESIN");
    return 0;
}

/* ---------- export ---------- */
extern char **environ;

static int bi_export(int argc, char **argv)
{
    if (argc == 1) {                         /* export seul : liste l'environnement */
        for (char **e = environ; *e; e++)
            printf("export %s\n", *e);
        return 0;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        if (is_assignment(argv[i])) {        /* export NOM=valeur */
            const char *eq = strchr(argv[i], '=');
            char *name = strndup(argv[i], (size_t)(eq - argv[i]));
            if (!name || var_set(name, eq + 1) < 0 || var_export(name) < 0) status = 1;
            free(name);
        } else {
            /* export NOM : on verifie que NOM est un identifiant valide */
            char probe[300];
            snprintf(probe, sizeof probe, "%s=", argv[i]);
            if (!is_assignment(probe)) {
                fprintf(stderr, "export: '%s' : identifiant invalide\n", argv[i]);
                status = 1;
            } else if (var_export(argv[i]) < 0) {
                status = 1;
            }
        }
    }
    return status;
}

/* ---------- table des builtins ---------- */
static const struct { const char *name; builtin_fn fn; } builtins[] = {
    { "go",     bi_go     },
    { "quit",   bi_quit   },
    { "path",   bi_path   },
    { "say",    bi_say    },
    { "info",   bi_info   },
    { "past",   bi_past   },
    { "osinfo", bi_osinfo },
    { "export", bi_export },
};

builtin_fn find_builtin(const char *name)
{
    for (size_t i = 0; i < sizeof builtins / sizeof *builtins; i++)
        if (strcmp(name, builtins[i].name) == 0)
            return builtins[i].fn;
    return NULL;
}
