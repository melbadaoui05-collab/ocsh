#ifndef ALIAS_H
#define ALIAS_H

/* "list" -> "ls", "show" -> "cat", ... ; retourne name si ce n'est pas un alias */
const char *resolve_alias(const char *name);

#endif /* ALIAS_H */
