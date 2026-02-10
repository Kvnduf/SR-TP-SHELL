/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"


int main()
{
	while (1) {
		struct cmdline *l;
		int i, j;
		pid_t pid = getpid();

		printf("shell> ");
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->seq != NULL && l->seq[0] != NULL && !strcmp(l->seq[0][0], "quit")) {
			Kill(pid, SIGTERM);
		}


		// Gestion d'une commande simple sans redirection ni pipe
		pid_t pid_child;

		pid_child = Fork();
		if (pid_child == 0) {
			execvp(l->seq[0][0], l->seq[0]);
			if (errno == ENOENT) {
				printf("%s: command not found\n", l->seq[0][0]);
				exit(127);
			}
			perror("execvp");
			exit(1);
		}
		waitpid(pid_child, NULL, 0);

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);

		/* Display each command of the pipe */
		for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
			for (j=0; cmd[j]!=0; j++) {
				printf("%s ", cmd[j]);
			}
			printf("\n");
		}
	}
}
