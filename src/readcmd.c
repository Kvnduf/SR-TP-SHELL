/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "readcmd.h"

/**
 * @brief Déclenche une erreur de mémoire et quitte le programme
 */
static void memory_error(void)
{
	errno = ENOMEM;
	perror(0);
	exit(1);
}

/**
 * @brief Alloue de la mémoire et gère les erreurs d'allocation
 * @param size La taille de mémoire à allouer
 * @return void* Un pointeur vers la mémoire allouée
 */
static void *xmalloc(size_t size)
{
	void *p = malloc(size);
	if (!p) memory_error();
	return p;
}

/**
 * @brief Réalloue de la mémoire et gère les erreurs d'allocation
 * 
 * @param ptr Pointeur vers la mémoire à réallouer
 * @param size Nouvelle taille de mémoire à allouer
 * @return void* Un pointeur vers la mémoire réallouée
 */
static void *xrealloc(void *ptr, size_t size)
{
	void *p = realloc(ptr, size);
	if (!p) memory_error();
	return p;
}

/**
 * @brief Lit une ligne de l'entrée standard et gère les erreurs d'entrée/sortie
 * @return char*  Un pointeur vers la ligne lue, ou NULL si l'entrée est fermée
 */
static char *readline(void)
{
	size_t buf_len = 16;
	char *buf = xmalloc(buf_len * sizeof(char));

	if (fgets(buf, buf_len, stdin) == NULL) {
		free(buf);
		return NULL;
	}
	
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	do {
		size_t l = strlen(buf);
		if ((l > 0) && (buf[l-1] == '\n')) {
			l--;
			buf[l] = 0;
			return buf;
		}
		if (buf_len >= (INT_MAX / 2)) memory_error();
		buf_len *= 2;
		buf = xrealloc(buf, buf_len * sizeof(char));
		if (fgets(buf + l, buf_len - l, stdin) == NULL) return buf;
	} while (1);
}


/**
 * @brief Divise une ligne en mots, en gérant les espaces, les tabulations et les caractères spéciaux (<, >, |)
 * 
 * @param line La ligne à diviser
 * @return char** Un tableau de chaînes de caractères représentant les mots de la ligne, terminé par un pointeur NULL
 * Exemple : pour la ligne "ls -l | grep a > output.txt", le tableau retourné serait ["ls", "-l", "|", "grep", "a", ">", "output.txt", NULL]
 */
static char **split_in_words(char *line)
{
	char *cur = line;
	char **tab = 0;
	size_t l = 0;
	char c;

	while ((c = *cur) != 0) {
		char *w = 0;
		char *start;
		switch (c) {
		case ' ':
		case '\t':
			/* Ignore any whitespace */
			cur++;
			break;
		case '<':
			w = "<";
			cur++;
			break;
		case '>':
			w = ">";
			cur++;
			break;
		case '|':
			w = "|";
			cur++;
			break;
		case '&':
			w = "&";
			cur++;
			break;
		default:
			/* Another word */
			start = cur;
			while (c) {
				c = *++cur;
				switch (c) {
				case 0:
				case ' ':
				case '\t':
				case '<':
				case '>':
				case '|':
				case '&':
					c = 0;
					break;
				default: ;
				}
			}
			w = xmalloc((cur - start + 1) * sizeof(char));
			strncpy(w, start, cur - start);
			w[cur - start] = 0;
		}
		if (w) {
			tab = xrealloc(tab, (l + 1) * sizeof(char *));
			tab[l++] = w;
		}
	}
	tab = xrealloc(tab, (l + 1) * sizeof(char *));
	tab[l++] = 0;
	return tab;
}

/**
 * @brief Libère la mémoire allouée pour une séquence de commandes
 * 
 * @param seq La séquence de commandes à libérer
 */
static void freeseq(char ***seq)
{
	int i, j;

	for (i=0; seq[i]!=0; i++) {
		char **cmd = seq[i];

		for (j=0; cmd[j]!=0; j++) free(cmd[j]);
		free(cmd);
	}
	free(seq);
}

/**
 * @brief Libère la mémoire allouée pour une structure cmdline, sans libérer la structure elle-même
 * 
 * @param s La structure cmdline à libérer
 */
static void freecmd(struct cmdline *s)
{
	if (s->in) free(s->in);
	if (s->out) free(s->out);
	if (s->seq) freeseq(s->seq);
}

/**
 * @brief Lit une ligne de commande à partir de l'entrée standard et l'analyse en une structure cmdline
 * @return struct cmdline* 
 */
struct cmdline *readcmd(void)
{
	static struct cmdline *static_cmdline = 0;
	struct cmdline *s = static_cmdline;
	char *line;
	char **words;
	int i;
	char *w;
	char **cmd;
	char ***seq;
	size_t cmd_len, seq_len;

	line = readline();
	if (line == NULL) {
		if (s) {
			freecmd(s);
			free(s);
		}
		return static_cmdline = 0;
	}

	cmd = xmalloc(sizeof(char *));
	cmd[0] = 0;
	cmd_len = 0;
	seq = xmalloc(sizeof(char **));
	seq[0] = 0;
	seq_len = 0;

	words = split_in_words(line);
	free(line);

	if (!s)
		static_cmdline = s = xmalloc(sizeof(struct cmdline));
	else
		freecmd(s);
	s->err = 0;
	s->in = 0;
	s->out = 0;
	s->out_append = 0;
	s->seq = 0;
	s->background = 0;

	i = 0;
	while ((w = words[i++]) != 0) {
		switch (w[0]) {
		case '<':
			/* Tricky : the word can only be "<" */
			if (s->in) {
				s->err = "only one input file supported";
				goto error;
			}
			if (words[i] == 0) {
				s->err = "filename missing for input redirection";
				goto error;
			}
			s->in = words[i++];
			break;
		case '>':
			/* Tricky : the word can only be ">" */
			if (s->out) {
				s->err = "only one output file supported";
				goto error;
			}
			/* Regarde si le prochain mot est aussi ">" pour le mode append (>>) */
			if (words[i] != 0 && words[i][0] == '>' && words[i][1] == '\0') {
				s->out_append = 1;
				i++; // Passe le mot ">" supplémentaire
				if (words[i] == 0) {
					s->err = "filename missing for output redirection";
					goto error;
				}
				s->out = words[i++];
			} else {
				s->out_append = 0;
				if (words[i] == 0) {
					s->err = "filename missing for output redirection";
					goto error;
				}
				s->out = words[i++];
			}
			break;
		case '|':
			/* Tricky : the word can only be "|" */
			if (cmd_len == 0) {
				s->err = "misplaced pipe";
				goto error;
			}

			seq = xrealloc(seq, (seq_len + 2) * sizeof(char **));
			seq[seq_len++] = cmd;
			seq[seq_len] = 0;

			cmd = xmalloc(sizeof(char *));
			cmd[0] = 0;
			cmd_len = 0;
			break;
		case '&':
			if (words[i] != 0) {
				s->err = "& must be at the end of the command";
				goto error;
			}
			s->background = 1;
			break;
		default:
			cmd = xrealloc(cmd, (cmd_len + 2) * sizeof(char *));
			cmd[cmd_len++] = w;
			cmd[cmd_len] = 0;
		}
	}

	if (cmd_len != 0) {
		seq = xrealloc(seq, (seq_len + 2) * sizeof(char **));
		seq[seq_len++] = cmd;
		seq[seq_len] = 0;
	} else if (seq_len != 0) {
		s->err = "misplaced pipe";
		i--;
		goto error;
	} else
		free(cmd);
	free(words);
	s->seq = seq;
	return s;
error:
	while ((w = words[i++]) != 0) {
		switch (w[0]) {
		case '<':
		case '>':
		case '|':
		case '&':
			break;
		default:
			free(w);
		}
	}
	free(words);
	freeseq(seq);
	for (i=0; cmd[i]!=0; i++) free(cmd[i]);
	free(cmd);
	if (s->in) {
		free(s->in);
		s->in = 0;
	}
	if (s->out) {
		free(s->out);
		s->out = 0;
	}
	return s;
}


int count_simple_commands(struct cmdline *cmd) {
	if (cmd == NULL || cmd->seq == NULL) {
		return -1;
	}

	int count = 0;
	while (cmd->seq[count] != NULL) {
		count++;
	}
	return count;
}