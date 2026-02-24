#include "csapp.h"
#include "execute.h"
#include "jobs.h"

#ifdef DEBUG
#define DEBUG_PRINT(...) printf("[DEBUG] : ") ;printf(__VA_ARGS__); 
#endif

/**
 * @brief Reconstruit la ligne de commande textuelle à partir d'une struct cmdline.
 * @param l Un pointeur vers un cmdline
 * @param buf Un buffer où stocker la ligne de commande reconstruite
 * @param bufsize La taille du buffer
 */
static void build_cmdline_str(struct cmdline *l, char *buf, size_t bufsize) {
    buf[0] = '\0';
    for (int i = 0; l->seq[i] != NULL; i++) {
        if (i > 0)
            strncat(buf," | ", bufsize - strlen(buf) - 1); // Ajouter un séparateur de pipe entre les commandes simples
        for (int j = 0; l->seq[i][j] != NULL; j++) {
            if (j > 0)
                strncat(buf, " ", bufsize - strlen(buf) - 1);
            strncat(buf, l->seq[i][j], bufsize - strlen(buf) - 1);
        }
    }
    if (l->background)
        strncat(buf, " &", bufsize - strlen(buf) - 1);
}

/* Gestion des signaux */

/**
 * @brief Traitant SIGCHLD
 * Pour chaque enfant terminé ou suspendu :
 *   - suspendu : on passe le job à JOB_STOPPED
 *   - terminé : on supprime le job de la table et on notifie Done si arrière-plan
 */
void sigchld_handler(int signum) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        job_t *j = get_job_by_pgid(pid);

        if (WIFSTOPPED(status)) { // Processus suspendu
            if (j != NULL) {
                j->state = JOB_STOPPED;
                // handler de signal => Sio_puts Sio_putl au lieu de printf 
                Sio_puts("\n[");
                Sio_putl(j->jid);
                Sio_puts("] ");
                Sio_putl((long)j->pgid);
                Sio_puts(" Stopped  ");
                Sio_puts(j->cmdline);
                Sio_puts("\n");
            }
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) { // Processus terminé
            if (j != NULL) {
                // Notifier uniquement si le job était en arrière-plan
                if (j->state == JOB_RUNNING || j->state == JOB_STOPPED) {
                    Sio_puts("[");
                    Sio_putl(j->jid);
                    Sio_puts("] ");
                    Sio_putl((long)j->pgid);
                    Sio_puts(" Done     ");
                    Sio_puts(j->cmdline);
                    Sio_puts("\n");
                }
                delete_job_by_pgid(pid);
            }
        }
    }
}

/**
 * @brief Traitant SIGTSTP
 * Envoie SIGTSTP au groupe de processus en foreground s'il existe.
 */
void sigtstp_handler(int signum) {
    job_t *fg = get_fg_job();
    if (fg != NULL) {
        kill(-(fg->pgid), SIGTSTP);
    }
}

/**
 * @brief Traitant SIGINT
 * Envoie SIGINT au groupe de processus en foreground s'il existe.
 */
void sigint_handler(int signum) {
    job_t *fg = get_fg_job();
    if (fg != NULL) {
        kill(-(fg->pgid), SIGINT);
    }
}


void setup_signals_handlers_shell() {
    jobs_init();
    pid_t shell_pgid;
    
    // Mettre le shell dans son propre groupe de processus + prendre le contrôle du terminal uniquement si stdin est un vrai terminal (car une erreur est provoqué avec le script de test).
    shell_pgid = getpid();
    if (isatty(STDIN_FILENO)) { // si stdin est un terminal => on prend le contrôle du terminal
        if (setpgid(shell_pgid, shell_pgid) < 0) { // mettre le shell dans son propre groupe de processus
            perror("setpgid");
            exit(1);
        }
        if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) { // place le groupe de processus du shell en foreground
            perror("tcsetpgrp");
            exit(1);
        }
    }
    
    // Ignorer SIGINT et SIGTSTP dans le shell
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT,  sigint_handler);
    Signal(SIGTSTP, sigtstp_handler);
    
    Signal(SIGTTOU, SIG_IGN);
    Signal(SIGTTIN, SIG_IGN);
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
void parent_cleanup(int fd_in, int fd_out, int background, pid_t* child_pids, int nb_cmds_executed, int* status, pid_t pgid) {
    // Fermer les descripteurs ouverts
    if (fd_in != STDIN_FILENO) Close(fd_in);
    if (fd_out != STDOUT_FILENO) Close(fd_out);

    if (!background) {
        // Attendre la fin du job de premier plan (terminal + sigsuspend + retour terminal)
        wait_for_fg_job(pgid);
    }
    free(child_pids);
}

void wait_for_fg_job(pid_t pgid) {
    
    if (pgid > 0 && isatty(STDIN_FILENO)) {
        if (tcsetpgrp(STDIN_FILENO, pgid) < 0) {
            if (errno != ESRCH && errno != EPERM)
                perror("tcsetpgrp to child (wait_for_fg_job)");
        }
    }

    // Attendre que le job de premier plan disparaisse du foreground
    sigset_t old_mask;
    jobs_block_sigchld(&old_mask);
    while (get_fg_job() != NULL) {
        Sigsuspend(&old_mask);
    }
    jobs_unblock_sigchld(&old_mask);

    // Rendre le terminal au shell car le job de premier plan a disparu du foreground
    if (isatty(STDIN_FILENO)) {
        if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0)
            perror("tcsetpgrp to shell (wait_for_fg_job)");
    }
}

int execute_command_line(struct cmdline *l) {
    int status = 0;
    int simple_cmds_nb = count_simple_commands(l);
    pid_t* child_pids = malloc(simple_cmds_nb * sizeof(pid_t)); // tableau pour stocker les PID des processus enfants
    int nb_cmds_executed = 0;
    pid_t pgid = 0; // ID de groupe de processus

    /* Construire la chaîne de commande pour la table de jobs */
    char cmdline_str[MAXCMDLEN];
    build_cmdline_str(l, cmdline_str, sizeof(cmdline_str));

    /*
     * Bloquer SIGCHLD :  un enfant ne peut pas terminer et mettre à jour la table avant que le parent appelle add_job.
     */
    sigset_t old_mask;
    jobs_block_sigchld(&old_mask);

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
            parent_cleanup(fd_in, fd_out, l->background, child_pids, nb_cmds_executed, &status, pgid);
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

    /*
     * En mode non interactif (sdriver.pl),
     * rediriger le stdout des jobs background vers /dev/null si aucune
     * redirection explicite n'est spécifiée.
     */
    if (l->background && !l->out && !isatty(STDOUT_FILENO)) {
        fd_out = open("/dev/null", O_WRONLY);
        if (fd_out < 0) {
            perror("/dev/null (stdout bg)");
            free(child_pids);
            return 1;
        }
    }
    if (l->out) {
        int flags = O_WRONLY | O_CREAT;
        if (l->out_append) {
            flags |= O_APPEND; // Mode append (>>)
        } else {
            flags |= O_TRUNC; // Mode truncate (>)
        }
        fd_out = open(l->out, flags, 0644);
        if (fd_out < 0) {
            perror(l->out);
            parent_cleanup(fd_in, fd_out, l->background, child_pids, nb_cmds_executed, &status, pgid);
            return 1;
        }
    }

    for (int i = 0; i < simple_cmds_nb; i++) {

        if (!is_last_simple_command(l, i)) { // si ce n'est pas la dernière commande simple => il faut créer un pipe
            curr_pipe = malloc(2 * sizeof(int));
            if (curr_pipe == NULL) {
                perror("malloc");
                parent_cleanup(fd_in, fd_out, l->background, child_pids, nb_cmds_executed, &status, pgid);
                return -1;
            }
            if (pipe(curr_pipe) < 0) {
                perror("pipe");
                free(curr_pipe);
                Close(previous_pipe[0]);
                Close(previous_pipe[1]);
                parent_cleanup(fd_in, fd_out, l->background, child_pids, nb_cmds_executed, &status, pgid);
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

            // Restaurer le masque de signaux pour les enfants
            jobs_unblock_sigchld(&old_mask);

            // Restaurer les handlers de signaux par défaut dans les enfants
            Signal(SIGINT, SIG_DFL);
            Signal(SIGTSTP, SIG_DFL);
            
            // Gestion gpid
            if (i == 0) {
                // Premier processus : crée son propre groupe
                setpgid(0, 0);
            } else {
                // Processus suivants : rejoindre le groupe du premier
                setpgid(0, child_pids[0]);
            }
            
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
        
        // Dans le parent : configurer le groupe de processus
        if (i == 0) {
            // Le premier enfant définit le pgid pour tout le job
            pgid = child_pids[0];
            setpgid(child_pids[0], pgid);
        } else {
            // Mettre les processus suivants dans le même groupe
            setpgid(child_pids[i], pgid);
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

    // Ajouter le job dans la table 
    if (pgid > 0) {
        job_state_t initial_state = l->background ? JOB_RUNNING : JOB_FOREGROUND;
        int jid = add_job(pgid, initial_state, cmdline_str);
        #ifdef DEBUG
        DEBUG_PRINT("Added job jid=%d pgid=%d state=%d cmdline='%s'\n", jid, (int)pgid, initial_state, cmdline_str);
        #endif
        if (l->background && jid > 0) { // On affiche immédiatement l'info du job si en background
            printf("[%d] %d\n", jid, (int)pgid);
        }
    }

    // la table est à jour => débloquer SIGCHLD maintenant 
    jobs_unblock_sigchld(&old_mask);

    parent_cleanup(fd_in, fd_out, l->background, child_pids, nb_cmds_executed, &status, pgid);
    return status;
}
