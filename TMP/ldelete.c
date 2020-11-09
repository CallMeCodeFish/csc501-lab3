#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <stdio.h>
#include <lock.h>

int ldelete(int lock) {
    STATWORD ps;
    disable(ps);

    if (lock < 0 || lock >= NLOCKS) {
        restore(ps);
        return SYSERR;
    }

    if (locktab[lock].status == LFREE) {
        restore(ps);
        return SYSERR;
    }

    lock_t *lptr = &locktab[lock];
    lptr->status = LFREE;
    lptr->type = 0;
    lptr->maxpriority = 0;

    // clear the process list of the lock
    plistnode_t *plcur = lptr->plhead->next;

    while (plcur != lptr->pltail) {
        // remove the lock from the lock list of the process
        struct pentry *pptr;
        pptr = &proctab[plcur->pid];

        llistnode_t *pllcur = pptr->pllhead->next;
        while (pllcur != pptr->plltail) {
            if (pllcur->lock == lock) break;
            pllcur = pllcur->next;
        }

        pllcur->prev->next = pllcur->next;
        pllcur->next->prev = pllcur->prev;

        freemem(pllcur, sizeof(llistnode_t));

        // free memory
        plistnode_t *todelete = plcur;
        plcur = plcur->next;
        freemem(todelete, sizeof(plistnode_t));
    }

    // put processes in the wait queue of the lock into ready queue
    qnode_t *qcur = lptr->qhead->next;
    while (qcur != lptr->qtail) {
        struct pentry *pptr;
        pptr = &proctab[qcur->pid];
        pptr->plockret = DELETED;
        pptr->plock = -1;
        ready(qcur->pid, RESCHNO);

        // free memory
        qnode_t *todelete = qcur;
        qcur = qcur->next;
        freemem(todelete, sizeof(qnode_t));
    }

    // set -1 to the callback array in the processes which used the lock
    int i;
    for (i = 0; i < NPROC; ++i) {
        struct pentry *pptr;
        pptr = &proctab[i];
        if (pptr->pstate != PRFREE && pptr->plockcallback[lock] == 1) {
            pptr->plockcallback[lock] = -1;
        }
    }

    restore(ps);
    return OK;
}