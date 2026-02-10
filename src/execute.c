#include "execute.h"
#include "readcmd.h"





int execute_command(struct cmdline *cmd, int background) {
    // Gestion d'une commande simple sans redirection ni pipe
    pid_t pid_child;


    int fd_in; // descripteur pour le fichier d'entrée
    int fd_out; // descripteur pour le fichier de sortie

    if (cmd->out) {
        fd_out = Open(cmd->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0) {
            perror("Open output file");
            return -1;
        }
    }

    pid_child = Fork();

    if (pid_child == 0) {
        if (cmd->in) {
            fd_in = Open(cmd->in, O_RDONLY, 0644);
            if (fd_in < 0) {
                perror("Open input file");
                return -1;
            }
            dup2(fd_in, STDIN_FILENO);
            Close(fd_in);
        }
        if (cmd->out) {
            fd_out = Open(cmd->out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) {
                perror("Open output file");
                return -1;
            }
            dup2(fd_out, STDOUT_FILENO);
            Close(fd_out);
        }
        execvp(cmd->seq[0][0], cmd->seq[0]);
        if (errno == ENOENT) {
            printf("%s: command not found\n", cmd->seq[0][0]);
            exit(127);
        }
        perror("execvp");
        exit(1);
    }
    waitpid(pid_child, NULL, 0);

    if (cmd->in) printf("in: %s\n", cmd->in);
    if (cmd->out) printf("out: %s\n", cmd->out);

    /* Display each command of the pipe */
    for (int i=0; cmd->seq[i]!=0; i++) {
        char **cmdpipe = cmd->seq[i];
        printf("seq[%d]: ", i);
        for (int j=0; cmdpipe[j]!=0; j++) {
            printf("%s ", cmdpipe[j]);
        }
        printf("\n");
    }
    return 0;
}
