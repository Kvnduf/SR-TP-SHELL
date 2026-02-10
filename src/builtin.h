#ifndef BUILTIN_H
#define BUILTIN_H

#include "readcmd.h"

/**
 * @brief Fonction qui exécute une commande intégrée (builtin) si elle est reconnue.
 * 
 * @param cmd Un pointeur vers une structure cmdline contenant la commande à exécuter.
 * @return int 
 * - Retourne 0 si la commande a été reconnue et exécutée avec succès.
 * - Retourne 1 si la commande n'est pas reconnue comme une commande intégrée (builtin).
 * - Retourne -1 si la commande a été reconnue mais a échoué lors de son exécution.
 */
int execute_builtin(struct cmdline *cmd);

#endif
