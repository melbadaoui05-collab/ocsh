#ifndef VARS_H
#define VARS_H

#include <sys/types.h>
#include "parser.h"

extern pid_t last_bg_pid;                           /* $! */

int         is_assignment(const char *word);        /* "NOM=valeur" ? */
const char *var_get(const char *name);              /* NULL si non definie ; gere aussi $? $! $$ */
int         var_set(const char *name, const char *value);
int         var_export(const char *name);           /* local -> environnement */
int         vars_try_assign(const Command *c);      /* 1 si la ligne n'est que des assignations */

#endif /* VARS_H */
