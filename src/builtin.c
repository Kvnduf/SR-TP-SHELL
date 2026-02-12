#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builtin.h"

int execute_builtin(struct cmdline *cmd) {
    if (cmd->seq == NULL || cmd->seq[0] == NULL || cmd->seq[0][0] == NULL) {
        return -1; // Pas un builtin
    }

    char *command = cmd->seq[0][0];
    // commande quit
    if ((strcmp(command, "quit") == 0) || (strcmp(command, "q") == 0)) {
        Kill(getpid(), SIGTERM);
        return 1; // renvoyer 1 pour indiquer une erreur si la terminaison du shell échoue
    }

    return -1; // Pas un builtin
}