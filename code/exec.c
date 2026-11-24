/*
 * exec.c -- execution d'un pipeline : fork / pipe / dup2 / execvp / wait
 */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "exec.h"
#include "builtins.h"
#include "alias.h"
#include "signals.h"
#include "vars.h"

int last_status = 0;                       /* $? */

/* Applique < > >> d'une commande sur les descripteurs 0 et 1.
 * Retourne 0 si ok, -1 sinon (message deja affiche). */
static int apply_redirs(const Command *c)
{
    if (c->in_file) {
        int fd = open(c->in_file, O_RDONLY);
        if (fd < 0) { fprintf(stderr, "ocsh: %s: %s\n", c->in_file, strerror(errno)); return -1; }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (c->out_file) {
        int flags = O_WRONLY | O_CREAT | (c->append ? O_APPEND : O_TRUNC);
        int fd = open(c->out_file, flags, 0644);
        if (fd < 0) { fprintf(stderr, "ocsh: %s: %s\n", c->out_file, strerror(errno)); return -1; }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    return 0;
}

/* Builtin seul (sans pipe, sans &) : execute DANS le shell, sinon
 * "go" et "quit" n'auraient aucun effet. On sauvegarde stdin/stdout pour
 * pouvoir les restaurer apres une eventuelle redirection. */
static int run_builtin_in_parent(const Command *c, builtin_fn fn)
{
    int save_in  = dup(STDIN_FILENO);
    int save_out = dup(STDOUT_FILENO);
    int status;

    if (apply_redirs(c) < 0) {
        status = 1;
    } else {
        status = fn(c->argc, (char **)c->argv);
        fflush(stdout);                    /* vider AVANT de restaurer stdout */
    }
    dup2(save_in,  STDIN_FILENO);
    dup2(save_out, STDOUT_FILENO);
    close(save_in);
    close(save_out);
    return status;
}

/* Attend un fils au premier plan ; retourne son code de retour. */
static int wait_foreground(pid_t pid)
{
    int st;
    while (waitpid(pid, &st, 0) < 0) {
        if (errno != EINTR) return 1;      /* EINTR : Ctrl+C pendant l'attente */
    }
    if (WIFEXITED(st))   return WEXITSTATUS(st);
    if (WIFSIGNALED(st)) return 128 + WTERMSIG(st);
    return 1;
}

int run_pipeline(Pipeline *p)
{
    /* cas 1 : un seul builtin, au premier plan -> dans le shell lui-meme */
    if (p->ncmds == 1 && !p->background) {
        builtin_fn fn = find_builtin(p->cmds[0].argv[0]);
        if (fn) return run_builtin_in_parent(&p->cmds[0], fn);
    }

    /* cas 2 : un fils par commande, relies par des pipes */
    int     n = p->ncmds;
    pid_t   pids[MAX_CMDS];
    int     started = 0;
    int     prev = -1;                     /* extremite lecture du pipe precedent */
    int     fd[2];

    for (int i = 0; i < n; i++) {
        Command *c = &p->cmds[i];

        if (i < n - 1 && pipe(fd) < 0) { perror("ocsh: pipe"); break; }

        pid_t pid = fork();
        if (pid < 0) {
            perror("ocsh: fork");
            if (i < n - 1) { close(fd[0]); close(fd[1]); }
            break;
        }

        if (pid == 0) {
            /* ---------- FILS ---------- */
            signal(SIGINT,  SIG_DFL);      /* Ctrl+C doit tuer le fils */
            signal(SIGQUIT, SIG_DFL);      /* le shell ignore SIGQUIT, pas ses fils */
            signal(SIGCHLD, SIG_DFL);

            /* en arriere-plan, le 1er processus ne lit pas le clavier */
            if (p->background && i == 0) {
                int nul = open("/dev/null", O_RDONLY);
                if (nul >= 0) { dup2(nul, STDIN_FILENO); close(nul); }
            }
            if (prev != -1) {              /* entree = pipe precedent */
                dup2(prev, STDIN_FILENO);
                close(prev);
            }
            if (i < n - 1) {               /* sortie = pipe suivant */
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }
            if (apply_redirs(c) < 0)       /* < et > l'emportent sur le pipe */
                _exit(1);

            builtin_fn fn = find_builtin(c->argv[0]);
            if (fn) {                      /* builtin dans un pipeline */
                int st = fn(c->argc, c->argv);
                fflush(stdout);
                _exit(st);
            }
            /* alias : "list" -> "ls". On remplace aussi argv[0] pour que les
             * messages d'erreur du programme disent "ls:" et pas "list:". */
            const char *prog = resolve_alias(c->argv[0]);
            c->argv[0] = (char *)prog;
            execvp(prog, c->argv);
            /* si on arrive ici, exec a echoue */
            fprintf(stderr, "ocsh: %s: %s\n", c->argv[0],
                    errno == ENOENT ? "commande introuvable" : strerror(errno));
            _exit(127);
        }

        /* ---------- PERE ---------- */
        pids[started++] = pid;
        if (prev != -1) close(prev);       /* le pere n'en a plus besoin */
        if (i < n - 1) { close(fd[1]); prev = fd[0]; }
    }
    if (prev != -1 && started < n) close(prev);   /* sortie anticipee sur erreur */

    if (started == 0) return 1;

    /* arriere-plan : on n'attend pas, SIGCHLD recoltera les fils */
    if (p->background) {
        for (int i = 0; i < started; i++) jobs_add(pids[i]);
        last_bg_pid = pids[started - 1];                 /* $! */
        fprintf(stderr, "[ocsh] en arriere-plan : pid %d\n", (int)pids[started - 1]);
        return 0;
    }

    /* premier plan : on attend tous les fils ; le code de retour est celui
     * de la derniere commande (comme bash) */
    int status = 0;
    for (int i = 0; i < started; i++) {
        int s = wait_foreground(pids[i]);
        if (i == started - 1) status = s;
    }
    if (started < n) status = 1;
    return status;
}
