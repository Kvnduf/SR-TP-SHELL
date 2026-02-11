#ifndef EXECUTE_H
#define EXECUTE_H

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"


/**
 * @brief Intialise les gestionnaires de signaux pour le shell
 * 
 */
void setup_signal_handlers();


/**
 * @brief Exécute une ligne de commande avec la gestion des redirections, des pipes et du background. 
 * 
 * @param l Un pointeur vers un cmdline contenant la ligne de commande à exécuter.
 * @return int valeur du status de la dernière commande exécutée, ou -1 en cas d'erreur d'exécution.
 */
int execute_command_line(struct cmdline *l);


#endif
