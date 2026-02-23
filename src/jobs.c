#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "jobs.h"

static job_t job_table[MAXJOBS];

/**
 * @brief Retourne l'indice de la première case libre, -1 si le tableau est plein.
 */
static int first_free_slot(void) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_table[i].jid == 0)
            return i;
    }
    return -1;
}

/**
 * @brief Détermine le plus petit jid disponible (>= 1).
 */
static int next_jid(void) {
    int max = 0;
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_table[i].jid > max)
            max = job_table[i].jid;
    }
    return max + 1;
}


void jobs_init(void) {
    for (int i = 0; i < MAXJOBS; i++) {
        job_table[i].jid   = 0;
        job_table[i].pgid  = 0;
        job_table[i].state = JOB_UNDEF;
        job_table[i].cmdline[0] = '\0';
    }
}

void jobs_block_sigchld(sigset_t *old_mask) {
    sigset_t mask;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);
    Sigprocmask(SIG_BLOCK, &mask, old_mask);
}

void jobs_unblock_sigchld(const sigset_t *old_mask) {
    Sigprocmask(SIG_SETMASK, old_mask, NULL);
}

int add_job(pid_t pgid, job_state_t state, const char *cmdline) {
    int slot = first_free_slot();
    if (slot < 0) {
        fprintf(stderr, "jobs: table pleine (MAXJOBS = %d)\n", MAXJOBS);
        return -1;
    }

    job_table[slot].jid   = next_jid();
    job_table[slot].pgid  = pgid;
    job_table[slot].state = state;
    strncpy(job_table[slot].cmdline, cmdline, MAXCMDLEN - 1);
    job_table[slot].cmdline[MAXCMDLEN - 1] = '\0';

    return job_table[slot].jid;
}

int delete_job_by_jid(int jid) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_table[i].jid == jid) {
            job_table[i].jid   = 0;
            job_table[i].pgid  = 0;
            job_table[i].state = JOB_UNDEF;
            job_table[i].cmdline[0] = '\0';
            return 0;
        }
    }
    return -1;
}

int delete_job_by_pgid(pid_t pgid) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_table[i].pgid == pgid && job_table[i].jid != 0) {
            job_table[i].jid   = 0;
            job_table[i].pgid  = 0;
            job_table[i].state = JOB_UNDEF;
            job_table[i].cmdline[0] = '\0';
            return 0;
        }
    }
    return -1;
}


job_t *get_job_by_jid(int jid) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_table[i].jid == jid)
            return &job_table[i];
    }
    return NULL;
}

job_t *get_job_by_pgid(pid_t pgid) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_table[i].pgid == pgid && job_table[i].jid != 0)
            return &job_table[i];
    }
    return NULL;
}

job_t *get_fg_job(void) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_table[i].state == JOB_FOREGROUND && job_table[i].jid != 0)
            return &job_table[i];
    }
    return NULL;
}

job_t *resolve_job_arg(const char *arg) {
    if (arg == NULL)
        return NULL;

    if (arg[0] == '%') {
        // Désignation par numéro de job : %N 
        int jid = atoi(arg + 1);
        return get_job_by_jid(jid);
    } else {
        // Désignation par PID (= pgid du groupe) 
        pid_t pgid = (pid_t)atoi(arg);
        return get_job_by_pgid(pgid);
    }
}

int set_job_state(int jid, job_state_t state) {
    job_t *j = get_job_by_jid(jid);
    if (j == NULL)
        return -1;
    j->state = state;
    return 0;
}

int set_job_state_by_pgid(pid_t pgid, job_state_t state) {
    job_t *j = get_job_by_pgid(pgid);
    if (j == NULL)
        return -1;
    j->state = state;
    return 0;
}


const char *job_state_str(job_state_t state) {
    switch (state) {
        case JOB_FOREGROUND: return "Foreground";
        case JOB_RUNNING: return "Running";
        case JOB_STOPPED: return "Stopped";
        default: return "Unknown";
    }
}

void list_jobs(void) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (job_table[i].jid != 0) {
            printf("[%d] %d %-10s %s\n", job_table[i].jid, (int)job_table[i].pgid, job_state_str(job_table[i].state), job_table[i].cmdline);
        }
    }
}
