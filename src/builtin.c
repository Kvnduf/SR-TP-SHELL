#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builtin.h"
#include "execute.h"
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

    // commande fg [%N | PID]
    if (strcmp(command, "fg") == 0) {
        char *arg = cmd->seq[0][1]; // recupérer l'argument optionnel
        job_t *j;

        // Bloquer SIGCHLD pendant la manipulation de la table de jobs
        sigset_t old_mask;
        jobs_block_sigchld(&old_mask);

        if (arg == NULL) {
            // si sans argument : prendre le dernier job stoppé ou en background
            j = NULL;
            
            // Prendre le job avec le plus grand jid qui n'est pas au foreground
            for (int i = 0; i < MAXJOBS; i++) {
                job_t *cand = get_job_by_jid(i + 1);
                if (cand && cand->state != JOB_FOREGROUND)
                    j = cand;
            }
        } else { // Un argument est fourni soit un job id soit un pgid
            j = resolve_job_arg(arg);
        }

        if (j == NULL) {
            fprintf(stderr, "fg: job not found\n");
            jobs_unblock_sigchld(&old_mask);
            return 1;
        }

        pid_t pgid = j->pgid;
        printf("%s\n", j->cmdline);
        fflush(stdout); // S'assurer que la ligne de commande est affichée avant de continuer
        j->state = JOB_FOREGROUND;
        kill(-pgid, SIGCONT); // Envoyer SIGCONT à tous les processus du groupe pour les faire passer au foreground

        jobs_unblock_sigchld(&old_mask);

        wait_for_fg_job(pgid);
        return 0;
    }

    // bg [%N | PID]
    if (strcmp(command, "bg") == 0) {
        char *arg = cmd->seq[0][1];
        if (arg == NULL) {
            fprintf(stderr, "bg: argument manquant (%%N ou PID)\n");
            return 1;
        }

        sigset_t old_mask;
        jobs_block_sigchld(&old_mask);

        job_t *j = resolve_job_arg(arg);
        if (j == NULL) {
            fprintf(stderr, "bg: job not found: %s\n", arg);
            jobs_unblock_sigchld(&old_mask);
            return 1;
        }

        j->state = JOB_RUNNING;
        pid_t pgid = j->pgid;
        printf("[%d] %d %s\n", j->jid, (int)pgid, j->cmdline);
        fflush(stdout);
        kill(-pgid, SIGCONT);

        jobs_unblock_sigchld(&old_mask);
        return 0;
    }

    // stop [%N | PID]
    if (strcmp(command, "stop") == 0) {
        char *arg = cmd->seq[0][1];
        if (arg == NULL) {
            fprintf(stderr, "stop: argument manquant (%%N ou PID)\n");
            return 1;
        }

        sigset_t old_mask;
        jobs_block_sigchld(&old_mask);

        job_t *j = resolve_job_arg(arg);
        if (j == NULL) {
            fprintf(stderr, "stop: job not found: %s\n", arg);
            jobs_unblock_sigchld(&old_mask);
            return 1;
        }

        pid_t pgid = j->pgid;
        // Envoyer SIGSTOP à tous les processus du groupe pour les suspendre
        kill(-pgid, SIGSTOP);

        jobs_unblock_sigchld(&old_mask);
        return 0;
    }

    // wait
    if (strcmp(command, "wait") == 0) {
        sigset_t old_mask;
        jobs_block_sigchld(&old_mask);
        while (has_running_jobs()) { // Tant qu'il y a des jobs en cours d'exécution ou stoppés, attendre les changements d'état
            Sigsuspend(&old_mask);
        }
        jobs_unblock_sigchld(&old_mask);
        return 0;
    }

    return -1; // Pas un builtin
}