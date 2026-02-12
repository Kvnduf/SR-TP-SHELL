# SR-TP-SHELL

## Compte-rendu

### Principales réalisations

L'implémentation d'un shell UNIX minimaliste en langage C. Les fonctionnalités suivantes ont été développées :

**Exécution de commandes**
- Exécution de commandes simples avec arguments
- Gestion des séquences de commandes avec pipes (`|`)
- Exécution en arrière-plan via l'opérateur `&` (et gestion du signal `SIGCHLD` pour éviter les processus zombies)

**Redirections d'entrée/sortie**
- Redirection d'entrée standard (`<`)
- Redirection de sortie standard (`>`)
- Redirection de sortie en mode ajout (`>>`)


**Commandes intégrées (builtins)**
- `quit` / `q` : terminaison propre du shell

**Architecture modulaire**
- Séparation du code en modules :
  - `builtin` : gestion des commandes intégrées
  - `execute` : exécution des commandes et gestion des processus
  - `readcmd` : analyse syntaxique de la ligne de commande (fourni par le sujet et adapté pour l'execution en arrière-plan)
  - `shell` : boucle principale du shell (processus père : lecture, analyse et creation de processus fils pour l'exécution)


### Description des tests effectués

Une suite de tests a été mise en place pour valider le bon fonctionnement du shell :


**Tests de redirections :**
- `test_redirection_in.txt` : Redirection d'entrée `<`
- `test_redirection_out.txt` : Redirection de sortie `>`
- `test_redirection_append1.txt`/`test_redirection_append2.txt` : Création et ajout successif avec `>>`


**Tests commandes pipe :**
- `test_redirection_avec_pipe.txt` : Combinaison de pipes `|`.

**Tests de robustesse**
- `test_redirection_complexe.txt` : Combinaison de plusieurs redirections et pipes.


**Tests commandes en arrière-plan**
...


**Tests d'erreurs**
...
