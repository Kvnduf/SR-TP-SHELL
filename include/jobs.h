#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
#include <signal.h>

#define MAXJOBS    100
#define MAXCMDLEN  512

/**
 * @brief État d'un job.
 *
 * JOB_UNDEF : case libre dans le tableau (pas de job actif)
 * JOB_FOREGROUND : exécuté au premier plan
 * JOB_RUNNING : exécuté en arrière-plan
 * JOB_STOPPED : suspendu (SIGTSTP / SIGSTOP)
 */
typedef enum {
    JOB_UNDEF      = 0,
    JOB_FOREGROUND = 1,
    JOB_RUNNING    = 2,
    JOB_STOPPED    = 3,
} job_state_t;

/**
 * @brief Structure de job.
 */
typedef struct {
    int          jid;
    pid_t        pgid;
    job_state_t  state;
    char         cmdline[MAXCMDLEN];
} job_t;


/**
 * @brief Initialise la table des jobs.
 */
void jobs_init();

/**
 * @brief Bloque SIGCHLD et sauvegarde l'ancien masque dans *old_mask.
 * @param old_mask Pointeur vers le masque de signaux à sauvegarder
 */
void jobs_block_sigchld(sigset_t *old_mask);

/**
 * @brief Rétablit le masque de signaux sauvegardé dans *old_mask.
 * @param old_mask Pointeur vers le masque de signaux à restaurer
 */
void jobs_unblock_sigchld(const sigset_t *old_mask);

/**
 * @brief Ajoute un nouveau job dans le tableau.
 * @param pgid Le pgid du groupe de processus du job
 * @param state L'état initial du job
 * @param cmdline La ligne de commande associée au job
 * @return jid du nouveau job (>= 1), ou -1 si le tableau est plein.
 *
 * IMPORTANT : appeler jobs_block_sigchld() AVANT cette fonction.
 */
int add_job(pid_t pgid, job_state_t state, const char *cmdline);

/**
 * @brief Supprime le job identifié par son jid et libère la case.
 * @param jid Le numéro du job à supprimer
 * @return 0 si trouvé et supprimé, -1 sinon.
 *
 * IMPORTANT : appeler jobs_block_sigchld() AVANT cette fonction.
 */
int delete_job_by_jid(int jid);

/**
 * @brief Supprime le job identifié par son pgid.
 * @param pgid Le pgid du groupe de processus du job à supprimer
 * @return 0 si trouvé et supprimé, -1 sinon.
 *
 * IMPORTANT : appeler jobs_block_sigchld() AVANT cette fonction.
 */
int delete_job_by_pgid(pid_t pgid);


/**
 * @brief Retourne un pointeur sur le job dont le jid correspond, NULL sinon.
 * @param jid Le numéro du job à rechercher
 * @return Pointeur sur le job trouvé, NULL si aucun job ne correspond.
 */
job_t *get_job_by_jid(int jid);


/**
 * @brief Retourne un pointeur sur le job dont le pgid correspond, NULL sinon.
 * @param pgid Le pgid du groupe de processus du job à rechercher
 * @return Pointeur sur le job trouvé, NULL si aucun job ne correspond.
 */
job_t *get_job_by_pgid(pid_t pgid);

/**
 * @brief Retourne le job actuellement au premier plan, NULL s'il n'y en a pas.
 * @return Pointeur sur le job au foreground, ou NULL s'il n'y en a pas.
 */
job_t *get_fg_job();

/**
 * @brief Résout un argument de la forme "%N" (numéro de job) ou "<pid>".
 * @param arg L'argument à résoudre
 * @return Pointeur sur le job trouvé, NULL si l'argument est invalide ou inconnu.
 */
job_t *resolve_job_arg(const char *arg);


/**
 * @brief Change l'état du job identifié par jid.
 * @param jid Le numéro du job à mettre à jour
 * @param state Le nouvel état du job
 * @return 0 si trouvé, -1 sinon.
 *
 * IMPORTANT : appeler jobs_block_sigchld() AVANT cette fonction.
 */
int set_job_state(int jid, job_state_t state);

/**
 * @brief Change l'état du job identifié par pgid.
 * @return 0 si trouvé, -1 sinon.
 */
int set_job_state_by_pgid(pid_t pgid, job_state_t state);


/**
 * @brief Affiche la liste des jobs actifs.
 */
void list_jobs();

/**
 * @brief Retourne 1 s'il existe au moins un job en état JOB_RUNNING ou JOB_STOPPED 0 sinon.
 */
int has_running_jobs(void);

/**
 * @brief Retourne une chaîne de caractères représentant l'état du job.
 * 
 * @param state L'état du job à convertir en chaîne
 * @return Chaîne de caractères correspondant à l'état du job ("Foreground", "Running", "Stopped", ou "Unknown").
 */
const char *job_state_str(job_state_t state);

#endif /* JOBS_H */
