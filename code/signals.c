/*
 * signals.c -- gestion des signaux : SIGINT, SIGQUIT, SIGCHLD (+ suivi des jobs &)
 *
 * Quand un fils se termine, il devient "zombie" tant que son pere n'a pas
 * appele wait(). Pour les commandes lancees avec &, le shell ne fait pas
 * wait() tout de suite : un handler SIGCHLD recolte ces fils (waitpid avec
 * WNOHANG) pour eviter les zombies.
 *
 * Le handler ne recolte QUE les pid enregistres ici, pour ne pas voler le
 * code de retour d'un fils au premier plan (attendu par run_pipeline).
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include "signals.h"

#define MAX_BG 64

/* Ctrl+C : le shell ne meurt pas ; il saute une ligne et reaffiche le prompt.
 * (Les fils remettent SIGINT par defaut : ils meurent, eux.) */
static void sigint_handler(int sig)
{
    (void)sig;
    const char nl = '\n';
    ssize_t r = write(STDOUT_FILENO, &nl, 1);
    (void)r;
}


static volatile pid_t bg[MAX_BG];     /* pid en cours, 0 = case libre   */
static volatile pid_t done[MAX_BG];   /* pid termines, pas encore annonces */
static volatile int   ndone = 0;

/* Recolte les fils d'arriere-plan termines (appele avec SIGCHLD bloque
 * ou depuis le handler lui-meme). */
static void reap(void)
{
    for (int i = 0; i < MAX_BG; i++) {
        if (bg[i] > 0) {
            int st;
            pid_t r = waitpid(bg[i], &st, WNOHANG);
            if (r == bg[i]) {
                if (ndone < MAX_BG) done[ndone++] = r;
                bg[i] = 0;
            }
        }
    }
}

static void sigchld_handler(int sig)
{
    (void)sig;
    int saved = errno;                /* waitpid peut modifier errno */
    reap();
    errno = saved;
}

void signals_init(void)
{
    struct sigaction sa;

    /* SIGINT sans SA_RESTART : getline() est interrompu, on reaffiche le prompt */
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    signal(SIGQUIT, SIG_IGN);

    /* SIGCHLD : recolte les fils d'arriere-plan (pas de zombies) */
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
}

static void block_chld(sigset_t *old)
{
    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s, SIGCHLD);
    sigprocmask(SIG_BLOCK, &s, old);
}

void jobs_add(pid_t pid)
{
    sigset_t old;
    block_chld(&old);
    for (int i = 0; i < MAX_BG; i++) {
        if (bg[i] == 0) { bg[i] = pid; break; }
    }
    reap();   /* le fils a pu finir avant l'enregistrement : SIGCHLD deja passe */
    sigprocmask(SIG_SETMASK, &old, NULL);
}

/* Annonce les jobs termines (appele avant chaque prompt). */
void jobs_report(void)
{
    sigset_t old;
    block_chld(&old);
    reap();
    for (int i = 0; i < ndone; i++)
        fprintf(stderr, "[ocsh] processus %d termine\n", (int)done[i]);
    ndone = 0;
    sigprocmask(SIG_SETMASK, &old, NULL);
}
