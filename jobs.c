/*
 * jobs.c -- processus en arriere-plan et SIGCHLD
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
#include "ocsh.h"

#define MAX_BG 64

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

void jobs_init(void)
{
    struct sigaction sa;
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
