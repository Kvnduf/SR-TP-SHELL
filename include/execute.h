#ifndef EXECUTE_H
#define EXECUTE_H

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"


/**
 * @brief Initialiser le comportement du shell vis-à-vis des signaux et du contrôle du terminal
 * 
 */
void setup_signals_handlers_shell();


/**
 * @brief Exécute une ligne de commande avec la gestion des redirections, des pipes et du background. 
 * 
 * @param l Un pointeur vers un cmdline contenant la ligne de commande à exécuter.
 * @return int valeur du status de la dernière commande exécutée, ou -1 en cas d'erreur d'exécution.
 */
int execute_command_line(struct cmdline *l);

/**
 * @brief Attend que le job de premier plan (pgid) disparaisse du foreground.
 *
 * @param pgid Le pgid du groupe de processus du job de premier plan
 */
void wait_for_fg_job(pid_t pgid);


#endif
