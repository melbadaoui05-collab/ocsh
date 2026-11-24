#ifndef BUILTINS_H
#define BUILTINS_H

typedef int (*builtin_fn)(int argc, char **argv);

builtin_fn find_builtin(const char *name);          /* NULL si ce n'est pas un builtin */

void history_add(const char *line);                 /* alimente "past" */
void history_print(void);

#endif /* BUILTINS_H */
