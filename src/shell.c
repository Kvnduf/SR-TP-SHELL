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
	while (1) {
		struct cmdline *l;
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
			// Rien (pour le moment)
		} else {
			execute_command_line(l, 0);
		}

	}
}
