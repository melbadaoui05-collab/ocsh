# ocsh — Mini-shell en C

Projet 3 — Systèmes d'Exploitation (UIR/ESIN)

`ocsh` est un interpréteur de commandes écrit en C qui utilise directement les appels système du noyau (`fork`, `execvp`, `wait`, `pipe`, `dup2`, `signal`...). Il implémente ses propres commandes renommées ainsi que l'opérateur de pipe `->`.

## Membres du groupe

| Nom | GitHub | Partie |
|-----|--------|--------|
| Nom 1 | @pseudo1 | Parser + REPL |
| Nom 2 | @pseudo2 | Exécution, redirections, pipes |
| Nom 3 | @pseudo3 | Builtins, alias, variables |
| Nom 4 | @pseudo4 | Signaux, historique, script, tests, rapport |

## Compilation et lancement

Nécessite Linux ou macOS (ou WSL / GitHub Codespaces) et `gcc`.

```bash
cd code
make          # compile et crée l'exécutable ./ocsh
./ocsh        # lance le shell
make clean    # supprime l'exécutable
```

## Commandes disponibles

### Builtins

| Bash | ocsh | Description |
|------|------|-------------|
| `cd` | `go` | Changer de répertoire |
| `exit` | `quit` | Quitter le shell |
| `pwd` | `path` | Afficher le répertoire courant |
| `echo` | `say` | Afficher du texte |
| `help` | `info` | Afficher l'aide |
| `history` | `past` | Afficher l'historique |
| pipe `\|` | pipe `->` | Relier deux commandes |
| — | `osinfo` | Affiche « Systèmes d'Exploitation -- UIR/ESIN » |

### Alias

| Bash | ocsh |
|------|------|
| `ls` | `list` |
| `cat` | `show` |
| `cp` | `copy` |
| `mv` | `move` |
| `rm` | `del` |
| `mkdir` | `newdir` |
| `touch` | `newfile` |

## Fonctionnalités (jalons)

- [ ] 1. Boucle REPL : lecture → découpage par `->` → `fork()` → `execvp()` → `wait()`
- [ ] 2. Builtins et alias ; redirections `>`, `>>`, `<` via `dup2`
- [ ] 3. Pipes `->` ; arrière-plan `&` ; signaux `SIGINT` / `SIGCHLD`
- [ ] 4. Variables et expansion : `$VAR`, `${VAR}`, `export`, `$?`, `$!`
- [ ] 5. Substitution `$(...)` / backticks ; `.ocshrc` ; historique et complétion
- [ ] 6. Mode script `ocsh script.ocsh` (`if` / `for` / `while`)
- [ ] Optionnel : job control `jobs`, `fg`, `bg`

(Cocher les cases au fur et à mesure : `- [x]`.)

## Exemples d'utilisation

```
ocsh> list
ocsh> go ..
ocsh> path
ocsh> say hello > f.txt
ocsh> show f.txt
ocsh> show f.txt -> wc -l
ocsh> sleep 10 &
ocsh> ps
ocsh> osinfo
```

## Structure du projet

```
ocsh/
├── README.md
├── report.pdf
├── code/
│   ├── Makefile
│   ├── main.c        # boucle REPL
│   ├── parser.c/.h   # découpage de la ligne de commande
│   ├── exec.c/.h     # fork, execvp, wait, dup2, pipe
│   ├── builtins.c/.h # go, quit, path, say, info, past, osinfo
│   ├── alias.c/.h    # list, show, copy, move, del, newdir, newfile
│   ├── vars.c/.h     # variables et expansion
│   ├── signals.c/.h  # SIGINT, SIGCHLD
│   └── tests/
```
