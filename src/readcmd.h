/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#ifndef __READCMD_H
#define __READCMD_H

/**
 * @brief Lit une ligne de commande à partir de l'entrée standard et la analyse en une structure cmdline
 * @return struct cmdline* 
 */
struct cmdline *readcmd(void);


/**
 * @brief Nombre de commandes simples dans une séquence de commandes
 * @param cmd Un pointeur vers une structure cmdline contenant la ligne de commande à analyser
 * @return int Le nombre de commandes simples dans la séquence de commandes, ou -1
 */
int count_simple_commands(struct cmdline *cmd);

/* Structure returned by readcmd() */
struct cmdline {
	char *err;	/* If not null, it is an error message that should be
			   displayed. The other fields are null. */
	char *in;	/* If not null : name of file for input redirection. */
	char *out;	/* If not null : name of file for output redirection. */
	char ***seq;	/* See comment below */
};

/* Field seq of struct cmdline :
A command line is a sequence of commands whose output is linked to the input
of the next command by a pipe. To describe such a structure :
A command is an array of strings (char **), whose last item is a null pointer.
A sequence is an array of commands (char ***), whose last item is a null
pointer.
When a struct cmdline is returned by readcmd(), seq[0] is never null.
*/
#endif
