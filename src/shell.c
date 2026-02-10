/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"
#include "builtin.h"


int main()
{
	while (1) {
		struct cmdline *l;
		int i, j;
		// pid_t pid = getpid();

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

		if (execute_builtin(l) <= 0) {
			continue;
		}


		// Gestion d'une commande simple sans redirection ni pipe
		pid_t pid_child;

		pid_child = Fork();

		int fd_in = STDIN_FILENO; // descripteur pour le fichier d'entrée
		int fd_out = STDOUT_FILENO; // descripteur pour le fichier de sortie

		// Cas cmd >
		i = 0;
		while (l->seq[0][i] != NULL) {

			// Cas >
			if (!strcmp(l->seq[0][i],">")) {
				fd_out = Open(l->seq[0][1+i],O_CREAT | O_WRONLY, 0644); // XXX = indice du XXX
				dup2(fd_out, STDOUT_FILENO);
			}
			// Cas >>
			if (!strcmp(l->seq[0][i],">>")) {
				fd_out = Open(l->seq[0][1+i],O_CREAT | O_WRONLY | O_APPEND, 0644); // XXX = indice du XXX
				dup2(fd_out, STDOUT_FILENO);
			}

			// Cas <
			if (!strcmp(l->seq[0][i],"<")) {
				fd_in = Open(l->seq[0][1+i],O_CREAT | O_RDONLY, 0644); // XXX = indice du XXX
				dup2(fd_in, STDIN_FILENO);
			}
			i++;
		}	


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
