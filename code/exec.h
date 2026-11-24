#ifndef EXEC_H
#define EXEC_H

#include "parser.h"

extern int last_status;                             /* code de retour ($?) */
int run_pipeline(Pipeline *p);                      /* fork / pipe / dup2 / execvp / wait */

void eval_line(const char *line);                   /* defini dans main.c : lex + parse + run */

#endif /* EXEC_H */
