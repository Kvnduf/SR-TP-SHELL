#ifndef BUILTIN_H
#define BUILTIN_H

#include "readcmd.h"

/**
 * @brief Fonction qui exécute une commande intégrée (builtin) si elle est reconnue.
 * 
 * @param cmd Un pointeur vers une structure cmdline contenant la commande à exécuter.
 * @return int la valeur de retour de la commande intégrée exécutée, ou -1 si la commande n'est pas reconnue comme un builtin.
 */
int execute_builtin(struct cmdline *cmd);

#endif
