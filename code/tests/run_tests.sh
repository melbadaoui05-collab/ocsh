#!/bin/bash
# Tests automatiques d'ocsh.
# Usage (depuis code/) :  make test      ou      ./tests/run_tests.sh
# Chaque test envoie des lignes a ocsh sur son entree standard et compare la sortie.

OCSH="$(cd "$(dirname "$0")/.." && pwd)/ocsh"
[ -x "$OCSH" ] || { echo "ocsh introuvable : lance 'make' d'abord"; exit 1; }

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
cd "$WORK" || exit 1

pass=0; fail=0

# check NOM ENTREE ATTENDU   -> compare la sortie standard seule
check() {
    local got; got=$(printf '%b' "$2" | "$OCSH" 2>/dev/null)
    if [ "$got" == "$3" ]; then pass=$((pass + 1)); echo "  ok    $1"
    else fail=$((fail + 1)); echo "  ECHEC $1"; echo "        attendu : $(printf '%q' "$3")"; echo "        obtenu  : $(printf '%q' "$got")"; fi
}

# check_err NOM ENTREE ATTENDU -> compare stdout + stderr (messages d'ocsh)
check_err() {
    local got; got=$(printf '%b' "$2" | "$OCSH" 2>&1)
    if [ "$got" == "$3" ]; then pass=$((pass + 1)); echo "  ok    $1"
    else fail=$((fail + 1)); echo "  ECHEC $1"; echo "        attendu : $(printf '%q' "$3")"; echo "        obtenu  : $(printf '%q' "$got")"; fi
}

echo "Jalon 1 : REPL, exec"
check     "say simple"                'say bonjour'                          'bonjour'
check     "commande externe"          'sleep 0\nsay ok'                       'ok'
check_err "commande introuvable"      'foo'                                  'ocsh: foo: commande introuvable'
check     "ligne vide et commentaire" '\n# rien\nsay ok'                     'ok'

echo "Jalon 2 : builtins, alias, redirections"
check     "osinfo"                    'osinfo'                               "Systèmes d'Exploitation -- UIR/ESIN"
check     "go + path"                 'go /tmp\npath'                        '/tmp'
check     "alias show"                'say salut > f.txt\nshow f.txt'        'salut'
check     "alias newfile/list"        'newfile a.txt\nlist a.txt'            'a.txt'
check     "alias copy/move/del"       'say x > a\ncopy a b\nmove b c\ndel a\nshow c' 'x'
check     "redirection > puis >>"     'say a > g.txt\nsay b >> g.txt\nshow g.txt' $'a\nb'
check     "redirection <"             'say dedans > h.txt\nshow < h.txt'     'dedans'
check     "guillemets"                'say "a  b" '"'"'c d'"'"''             'a  b c d'

echo "Jalon 3 : pipes, arriere-plan, signaux"
check     "pipe ->"                   'say hello -> tr a-z A-Z'              'HELLO'
check     "pipe chaine"               'say a -> tr a b -> tr b c'            'c'
check     "pipe + redirection"        'say hello -> tr a-z A-Z > p.txt\nshow p.txt' 'HELLO'
check_err "pipe sans commande"        'show f.txt ->'                        'ocsh: erreur de syntaxe : commande manquante'
check     "arriere-plan rend la main" 'sleep 0.2 &\nsay apres'               'apres'

echo "Jalon 4 : variables et expansion"
check     "assignation + \$X"         'X=5\nsay $X ${X}abc "$X"'            '5 5abc 5'
check     "\$ echappe"                'X=5\nsay \\$X'                        '$X'
check     "variable inconnue"         'say a $UNDEF b'                       'a b'
check     "inconnue entre guillemets" 'say a "$UNDEF" b'                     'a  b'
check     "valeur avec espaces"       'M="un deux"\nsay "$M"'                'un deux'
check     "locale non exportee"       'FOO=hello\nsh -c "echo [\\$FOO]"'     '[]'
check     "export NOM"                'FOO=hello\nexport FOO\nsh -c "echo [\\$FOO]"' '[hello]'
check     "export NOM=valeur"         'export BAR=w\nsh -c "echo \\$BAR"'    'w'
check     "\$? apres echec"           'list /nonexistent\nsay $?'           '2'
check     "\$? apres succes"          'say ok\nsay $?'                       $'ok\n0'
check     "\$! apres &"               'say [$!]\nsleep 0.2 &\nsay ok'        '[]
ok'
check_err "\${ non ferme"             'say ${X'                              'ocsh: ${...} non ferme (ou nom trop long)'
check_err "export invalide"           'export 1abc'                          "export: '1abc' : identifiant invalide"

echo
echo "Resultat : $pass reussis, $fail echecs"
[ "$fail" -eq 0 ]
