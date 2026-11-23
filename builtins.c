/*
 * builtins.c -- commandes integrees et table des alias
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
#include "ocsh.h"

/* ---------- table des alias : nom ocsh -> commande bash ---------- */
static const struct { const char *ocsh; const char *bash; } aliases[] = {
    { "list",    "ls"    },
    { "show",    "cat"   },
    { "copy",    "cp"    },
    { "move",    "mv"    },
    { "del",     "rm"    },
    { "newdir",  "mkdir" },
    { "newfile", "touch" },
};

const char *resolve_alias(const char *name)
{
    for (size_t i = 0; i < sizeof aliases / sizeof *aliases; i++)
        if (strcmp(name, aliases[i].ocsh) == 0)
            return aliases[i].bash;
    return name;                    /* pas un alias : on garde le nom */
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
    puts("  osinfo       informations sur le cours\n");
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

/* ---------- table des builtins ---------- */
static const struct { const char *name; builtin_fn fn; } builtins[] = {
    { "go",     bi_go     },
    { "quit",   bi_quit   },
    { "path",   bi_path   },
    { "say",    bi_say    },
    { "info",   bi_info   },
    { "past",   bi_past   },
    { "osinfo", bi_osinfo },
};

builtin_fn find_builtin(const char *name)
{
    for (size_t i = 0; i < sizeof builtins / sizeof *builtins; i++)
        if (strcmp(name, builtins[i].name) == 0)
            return builtins[i].fn;
    return NULL;
}
