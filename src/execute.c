#include "execute.h"
#include "readcmd.h"

/**
 * @brief Crée un processus qui exécute une commande simple avec redirection d'entrée/sortie et attend sa terminaison. 
 * @param cmd_simple Un tableau de strings représentant la commande simple à exécuter 
 * @param fd_in Descripteur de fichier pour la redirection d'entrée
 * @param fd_out Descripteur de fichier pour la redirection de sortie
 * @return int 
 */
int execute_simple_command(char** cmd_simple, int fd_in, int fd_out) {
    pid_t pid_child;
    int status;

    pid_child = Fork();

    if (pid_child == 0) {
        if (fd_in != STDIN_FILENO) {
            dup2(fd_in, STDIN_FILENO);
        }
        if (fd_out != STDOUT_FILENO) {
            dup2(fd_out, STDOUT_FILENO);
        }
        execvp(cmd_simple[0], cmd_simple);
        if (errno == ENOENT) {
            printf("%s: command not found\n", cmd_simple[0]);
            exit(127);
        }
        perror("execvp");
        exit(1);
    }
    waitpid(pid_child, &status, 0);
    return status;
    
}


int execute_command_line(struct cmdline *l, int background) {
    int status;

    int fd_in = STDIN_FILENO; // descripteur pour le fichier d'entrée
    int fd_out = STDOUT_FILENO; // descripteur pour le fichier de sortie

    if (l->in) {
        fd_in = open(l->in, O_RDONLY, 0644);
        if (fd_in < 0) {
            perror(l->in);
            return -1;
        }
    }
    if (l->out) {
        fd_out = open(l->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0) {
            perror(l->out);
            return -1;
        }
    }
    status = execute_simple_command(l->seq[0], fd_in, fd_out);
    
    // Fermer les descripteurs ouverts
    if (fd_in != STDIN_FILENO) Close(fd_in);
    if (fd_out != STDOUT_FILENO) Close(fd_out);
    

    if (l->in) printf("in: %s\n", l->in);
    if (l->out) printf("out: %s\n", l->out);

    /* Display each command of the pipe */
    for (int i=0; l->seq[i]!=0; i++) {
        char **cmd = l->seq[i];
        printf("seq[%d]: ", i);
        for (int j=0; cmd[j]!=0; j++) {
            printf("%s ", cmd[j]);
        }
        printf("\n");
    }
    return status;
}
