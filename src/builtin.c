#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builtin.h"
#include "jobs.h"

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

    // commande jobs
    if (strcmp(command, "jobs") == 0) {
        list_jobs();
        return 0;
    }

    /* fg / bg / stop */
    /*
     * TODO étape suivante :
     *   fg [%N | PID]  -> SIGCONT + mettre en FG
     *   bg [%N | PID]  -> SIGCONT + laisser en BG
     *   stop [%N | PID]-> SIGSTOP
     *
     * Récupération de l'argument :
     *   char *arg = (cmd->seq[0][1] != NULL) ? cmd->seq[0][1] : NULL;
     *   job_t *j  = resolve_job_arg(arg);
     */

    return -1; // Pas un builtin
}