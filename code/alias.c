/*
 * alias.c -- table des alias : nom ocsh -> commande bash
 *
 * Un alias n'est PAS un builtin : exec.c le traduit juste avant execvp()
 * ("list" devient "ls"), et c'est le vrai programme qui s'execute.
 */
#include <string.h>
#include "alias.h"

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
