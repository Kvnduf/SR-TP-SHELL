#include "execute.h"
#include "readcmd.h"

/**
 * @brief Exécute une commande simple avec redirection d'entrée/sortie. 
 * @param cmd_simple Un tableau de strings représentant la commande simple à exécuter 
 * @param fd_in Descripteur de fichier pour la redirection d'entrée
 * @param fd_out Descripteur de fichier pour la redirection de sortie
 * @param fd_err Descripteur de fichier pour la redirection d'erreur
 * @return void
 */
void execute_simple_command(char** cmd_simple, int fd_in, int fd_out, int fd_err) {
    if (fd_in != STDIN_FILENO) {
        dup2(fd_in, STDIN_FILENO);
    }
    if (fd_out != STDOUT_FILENO) {
        dup2(fd_out, STDOUT_FILENO);
    }
    if (fd_err != STDERR_FILENO) {
        dup2(fd_err, STDERR_FILENO);
    }
    
    execvp(cmd_simple[0], cmd_simple);
    if (errno == ENOENT) {
        printf("%s: command not found\n", cmd_simple[0]);
        exit(127);
    }
    perror("execvp");
    exit(1);
}


/**
 * @brief Si la commande simple est précédée d'un pipe
 * 
 * @param l Un pointeur vers un cmdline contenant la ligne de commande à exécuter.
 * @param i L'indice de la commande simple dans la séquence de commandes à exécuter.
 * @return int 1 si la commande simple est précédée d'un pipe, 0 sinon.
 */
int is_last_simple_command(struct cmdline *l, int i) {
    return l->seq[i + 1] == NULL; // Si la commande suivante dans la séquence n'est pas nulle, alors il y a un pipe
}


// Cleanup de la fonction execute_command_line
void parent_cleanup(int fd_in, int fd_out, int fd_err, int background, pid_t* child_pids, int nb_cmds_executed, int* status) {
    // Fermer les descripteurs ouverts
    if (fd_in != STDIN_FILENO) Close(fd_in);
    if (fd_out != STDOUT_FILENO) Close(fd_out);
    if (fd_err != STDERR_FILENO) Close(fd_err);

    if (background) return;
    // Attendre la fin de tous les processus enfants si la commande n'est pas en arrière-plan
    for (int i = 0; i < nb_cmds_executed; i++) {
        waitpid(child_pids[i], status, 0);
    }
    free(child_pids);
}

int execute_command_line(struct cmdline *l, int background) {
    int status = 0;
    int simple_cmds_nb = count_simple_commands(l);
    pid_t* child_pids = malloc(simple_cmds_nb * sizeof(pid_t)); // tableau pour stocker les PID des processus enfants
    int nb_cmds_executed = 0;

    // Ouverture des fichiers pour les redirections
    int fd_in = STDIN_FILENO; // descripteur pour le fichier d'entrée
    int fd_out = STDOUT_FILENO; // descripteur pour le fichier de sortie
    int fd_err = STDERR_FILENO; // descripteur pour le fichier d'erreur

    int curr_fd_in;
    int curr_fd_out;
    int* curr_pipe = NULL;
    int* previous_pipe = NULL;

    if (l->in) {
        fd_in = open(l->in, O_RDONLY, 0644);
        if (fd_in < 0) {
            perror(l->in);
            parent_cleanup(fd_in, fd_out, fd_err, background, child_pids, nb_cmds_executed, &status);
            return 1;
        }
    }
    if (l->out) {
        int flags = O_WRONLY | O_CREAT;
        if (l->append) {
            flags |= O_APPEND;  // Mode append (>>)
        } else {
            flags |= O_TRUNC;   // Mode truncate (>)
        }
        fd_out = open(l->out, flags, 0644);
        if (fd_out < 0) {
            perror(l->out);
            parent_cleanup(fd_in, fd_out, fd_err, background, child_pids, nb_cmds_executed, &status);
            return 1;
        }
    }
    if (l->err) {
        fd_err = open(l->err, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_err < 0) {
            perror(l->err);
            parent_cleanup(fd_in, fd_out, fd_err, background, child_pids, nb_cmds_executed, &status);
            return 1;
        }
    }

    for (int i = 0; i < simple_cmds_nb; i++) {

        if (!is_last_simple_command(l, i)) {
            curr_pipe = malloc(2 * sizeof(int));
            if (curr_pipe == NULL) {
                perror("malloc");
                parent_cleanup(fd_in, fd_out, fd_err, background, child_pids, nb_cmds_executed, &status);
                return -1;
            }
            if (pipe(curr_pipe) < 0) {
                perror("pipe");
                free(curr_pipe);
                Close(previous_pipe[0]);
                Close(previous_pipe[1]);
                parent_cleanup(fd_in, fd_out, fd_err, background, child_pids, nb_cmds_executed, &status);
                return -1;
            }
        }

        child_pids[i] = Fork();
        if (child_pids[i] == 0) {
            // Entrée
            if (i == 0) { // première commande simple => redirection d'entrée
                curr_fd_in = fd_in;
            } else { // récup le dernier pipe
                curr_fd_in = previous_pipe[0];
                Close(previous_pipe[1]);
            }

            // Sortie
            if (is_last_simple_command(l, i)) { // dernière commande simple => redirection de sortie
                curr_fd_out = fd_out;
            } else {
                curr_fd_out = curr_pipe[1];
                Close(curr_pipe[0]);
            }
            execute_simple_command(l->seq[i], curr_fd_in, curr_fd_out, fd_err);
        }
        nb_cmds_executed++;
        if (previous_pipe) {
            Close(previous_pipe[0]);
            Close(previous_pipe[1]);
            free(previous_pipe);
        }
        previous_pipe = curr_pipe;
        curr_pipe = NULL;
    }

    parent_cleanup(fd_in, fd_out, fd_err, background, child_pids, nb_cmds_executed, &status);
    return status;
}
