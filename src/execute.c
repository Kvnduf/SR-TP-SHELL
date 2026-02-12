#include "execute.h"

#ifdef DEBUG
#define DEBUG_PRINT(...) printf("[DEBUG] : ") ;printf(__VA_ARGS__); 
#endif

// Gestion des signaux

// Handler pour SIGCHLD
void sigchld_handler(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}


// Fonction pour activer la gestion des signaux
void setup_signal_handlers() {
    Signal(SIGCHLD, sigchld_handler);
}



/**
 * @brief Exécute une commande simple avec redirection d'entrée/sortie. 
 * @param cmd_simple Un tableau de strings représentant la commande simple à exécuter 
 * @param fd_in Descripteur de fichier pour la redirection d'entrée
 * @param fd_out Descripteur de fichier pour la redirection de sortie
 * @return void
 */
void execute_simple_command(char** cmd_simple, int fd_in, int fd_out) {
    #ifdef DEBUG
    DEBUG_PRINT("Executing simple command: %s, pid : %d, fd_in : %d, fd_out : %d\n", cmd_simple[0], getpid(), fd_in, fd_out); 
    #endif

    if (fd_in != STDIN_FILENO) {
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }
    if (fd_out != STDOUT_FILENO) {
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
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
void parent_cleanup(int fd_in, int fd_out, int background, pid_t* child_pids, int nb_cmds_executed, int* status) {
    // Fermer les descripteurs ouverts
    if (fd_in != STDIN_FILENO) Close(fd_in);
    if (fd_out != STDOUT_FILENO) Close(fd_out);
    
    if (!background) {
        for (int i = 0; i < nb_cmds_executed; i++) {
            #ifdef DEBUG
            DEBUG_PRINT("Parent waiting PID %d\n", child_pids[i]); 
            #endif
            waitpid(child_pids[i], status, 0);
            #ifdef DEBUG
            DEBUG_PRINT("Child PID %d finished with status %d\n", child_pids[i], *status); 
            #endif
        }
    }
    free(child_pids);
}

int execute_command_line(struct cmdline *l) {
    int status = 0;
    int simple_cmds_nb = count_simple_commands(l);
    pid_t* child_pids = malloc(simple_cmds_nb * sizeof(pid_t)); // tableau pour stocker les PID des processus enfants
    int nb_cmds_executed = 0;

    // Ouverture des fichiers pour les redirections
    int fd_in = STDIN_FILENO; // descripteur pour le fichier d'entrée
    int fd_out = STDOUT_FILENO; // descripteur pour le fichier de sortie

    int curr_fd_in;
    int curr_fd_out;
    int* curr_pipe = NULL;
    int* previous_pipe = NULL;

    if (l->in) {
        fd_in = open(l->in, O_RDONLY, 0644);
        if (fd_in < 0) {
            perror(l->in);
            parent_cleanup(fd_in, fd_out, l->background, child_pids, nb_cmds_executed, &status);
            return 1;
        }
    } 
    // Sinon si c'est en background sans redirection explicite utiliser /dev/null
    else if (l->background) {
        fd_in = open("/dev/null", O_RDONLY);
        if (fd_in < 0) {
            perror("/dev/null");
            free(child_pids);
            return 1;
        }
    }
    if (l->out) {
        int flags = O_WRONLY | O_CREAT;
        if (l->out_append) {
            flags |= O_APPEND;  // Mode append (>>)
        } else {
            flags |= O_TRUNC;   // Mode truncate (>)
        }
        fd_out = open(l->out, flags, 0644);
        if (fd_out < 0) {
            perror(l->out);
            parent_cleanup(fd_in, fd_out, l->background, child_pids, nb_cmds_executed, &status);
            return 1;
        }
    }

    for (int i = 0; i < simple_cmds_nb; i++) {

        if (!is_last_simple_command(l, i)) {
            curr_pipe = malloc(2 * sizeof(int));
            if (curr_pipe == NULL) {
                perror("malloc");
                parent_cleanup(fd_in, fd_out, l->background, child_pids, nb_cmds_executed, &status);
                return -1;
            }
            if (pipe(curr_pipe) < 0) {
                perror("pipe");
                free(curr_pipe);
                Close(previous_pipe[0]);
                Close(previous_pipe[1]);
                parent_cleanup(fd_in, fd_out, l->background, child_pids, nb_cmds_executed, &status);
                return -1;
            }
            #ifdef DEBUG
            DEBUG_PRINT("Created pipe, command %d: read : %d, write : %d\n", i, curr_pipe[0], curr_pipe[1]); 
            #endif
        }

        child_pids[i] = Fork();
        if (child_pids[i] == 0) {
            #ifdef DEBUG
            DEBUG_PRINT("Child process %d created for command %d\n", getpid(), i); 
            #endif
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
            execute_simple_command(l->seq[i], curr_fd_in, curr_fd_out);
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

    parent_cleanup(fd_in, fd_out, l->background, child_pids, nb_cmds_executed, &status);
    return status;
}
