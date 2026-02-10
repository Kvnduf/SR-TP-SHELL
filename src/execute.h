#ifndef EXECUTE_H
#define EXECUTE_H

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"

/**
 * @brief Exécute une ligne de commande avec la gestion des redirections, des pipes et du background. 
 * 
 * @param l Un pointeur vers un cmdline contenant la ligne de commande à exécuter.
 * @param background Indique si la commande doit s'exécuter en arrière-plan (1) ou au premier plan (0).
 * @return int valeur du status de la dernière commande exécutée, ou -1 en cas d'erreur d'exécution.
 */
int execute_command_line(struct cmdline *l, int background);


#endif
