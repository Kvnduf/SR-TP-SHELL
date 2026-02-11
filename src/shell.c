/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"
#include "builtin.h"
#include "execute.h"


int main()
{
	int status;
	setup_signal_handlers();
	while (1) {
		struct cmdline *l;

		// Affichage du prompt
		printf("shell> ");

		// Lecture de la ligne de commande
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}
		
		// analyse de la ligne de commande
		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}
		status = execute_builtin(l);
		if (status == -1) {
			status = execute_command_line(l);
		}
	}
}
