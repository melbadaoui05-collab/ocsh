OCSH

Mini-shell développé en C dans le cadre du module Systèmes d'Exploitation.


Fonctionnalités

Builtins : go, quit, path, say, info, past, osinfo

Alias : list, show, copy, move, del, newdir, newfile

Redirections : <, >, >>

Pipe personnalisé : ->

Exécution en arrière-plan : &

Gestion des signaux SIGINT et SIGCHLD

Compilation

make

Exécution

./ocsh


Exemple

osinfo

say hello -> tr a-z A-Z

say bonjour > test.txt

show test.txt

sleep 5 &

past


Auteurs

El Badaoui Mohamed

Attouch Douae

El Alama Marwa

Groupe Triple Prompt
