/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "builtin.h"
#include "execute.h"

#ifdef DEBUG
#define DEBUG_PRINT(...) printf("[DEBUG] : ") ;printf(__VA_ARGS__); 
#endif


int main()
{
	#ifdef DEBUG
	DEBUG_PRINT("Starting shell with parent PID %d\n", getpid()); 
	#endif

	int status;
	setup_signals_handlers_shell();
	while (1) {
		struct cmdline *l;

		// Affichage du prompt
		printf("shell> ");
		fflush(stdout); // On doit s'assurer que le prompt est affiché avant de lire la commande

		// Lecture de la ligne de commande
		l = readcmd();

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
