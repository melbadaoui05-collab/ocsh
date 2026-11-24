#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>

void signals_init(void);        /* installe SIGINT, SIGQUIT, SIGCHLD (a appeler au demarrage) */
void jobs_add(pid_t pid);       /* enregistre un processus lance avec & */
void jobs_report(void);         /* annonce les jobs termines (avant chaque prompt) */

#endif /* SIGNALS_H */
