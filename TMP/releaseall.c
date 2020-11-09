#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <stdio.h>
#include <lock.h>

int releaseall(int count, long locks) {
    STATWORD ps;
    disable(ps);

    int ret = OK;

    int *a;
    a = (int *) (&locks);

    struct pentry *pptr = &proctab[currpid];
    int maxpriority = 0;

    int i;
    for (i = 0; i < count; ++i) {
        int lock = a[i];

        if (release(lock, currpid) == SYSERR) ret = SYSERR;

        // if (lock < 0 || lock >= NLOCKS) {
        //     ret = SYSERR;
        //     continue;
        // }

        // lock_t *lptr = &locktab[lock];

        // if (lptr->status != LLOCKED) {
        //     ret = SYSERR;
        //     continue;
        // }

        // plistnode_t *cur = lptr->plhead->next;
        // plistnode_t *end = lptr->pltail;
        // while (cur != end) {
        //     if (cur->pid == currpid) break;
        //     cur = cur->next;
        // }

        // if (cur == end) {
        //     ret = SYSERR;
        //     continue;
        // }

        // // remove current process from the process list of the lock
        // cur->prev->next = cur->next;
        // cur->next->prev = cur->prev;
        // freemem(cur, sizeof(plistnode_t));

        // // remove the lock from the lock list held by current process
        // llistnode_t *lcur = pptr->pllhead->next;
        // llistnode_t *lend = pptr->plltail;
        // while (lcur != lend) {
        //     if (lcur->lock == lock) break;
        //     lcur = lcur->next;
        // }
        // lcur->prev->next = lcur->next;
        // lcur->next->prev = lcur->prev;
        // freemem(lcur, sizeof(llistnode_t));

        // // keep track of maxpriority
        // if (maxpriority < lptr->maxpriority) {
        //     maxpriority = lptr->maxpriority;
        // }
        
        // // no other reader process is holding the lock
        // if (lptr->plhead->next == lptr->pltail) {
        //     qnode_t *nextproc = lptr->qhead->next;

        //     if (nextproc != lptr->qtail) {
        //         // put the next waiting process of the lock into ready queue
        //         lptr->type = nextproc->type;

        //         if (nextproc->type == READ) {
        //             int targetpriority = nextproc->priority;
        //             while (nextproc != lptr->qtail && nextproc->priority == targetpriority) {
        //                 // remove current process from the wait queue
        //                 lptr->qhead->next = nextproc->next;
        //                 nextproc->next->prev = lptr->qhead;

        //                 // insert current process into the list holding the lock
        //                 insert_node_to_proc_list(lock, nextproc->pid);
        //                 insert_node_to_proc_lock_list(lock, nextproc->pid);
        //                 proctab[nextproc->pid].plock = -1;
                        
        //                 // ramp up the priority
        //                 if (proctab[nextproc->pid].pinh == 0 && proctab[nextproc->pid].pprio < lptr->maxpriority || proctab[nextproc->pid].pinh != 0 && proctab[nextproc->pid].pinh < lptr->maxpriority) {
        //                     proctab[nextproc->pid].pinh = lptr->maxpriority;
        //                 }

        //                 ready(nextproc->pid, RESCHNO);

        //                 // free memory
        //                 qnode_t *todelete = nextproc;
        //                 nextproc = nextproc->next;
        //                 freemem(todelete, sizeof(qnode_t));
        //             }

        //             // recalculate the maximum scheduling process of the lock
        //             update_max_sched_priority(lock);
        //         } else {
        //             // writer

        //             // remove current process from the wait queue
        //             lptr->qhead->next = nextproc->next;
        //             nextproc->next->prev = lptr->qhead;

        //             // insert current process into the list holding the lock
        //             insert_node_to_proc_list(lock, nextproc->pid);
        //             insert_node_to_proc_lock_list(lock, nextproc->pid);
        //             proctab[nextproc->pid].plock = -1;

        //             // ramp up the priority
        //             if (proctab[nextproc->pid].pinh == 0 && proctab[nextproc->pid].pprio < lptr->maxpriority || proctab[nextproc->pid].pinh != 0 && proctab[nextproc->pid].pinh < lptr->maxpriority) {
        //                 proctab[nextproc->pid].pinh = lptr->maxpriority;
        //             }

        //             ready(nextproc->pid, RESCHNO);

        //             // recalculate the maximum scheduling process of the lock
        //             update_max_sched_priority(lock);

        //             // free memory
        //             freemem(nextproc, sizeof(qnode_t));
        //         }
        //     } else {
        //         // set the status of the lock to LUNLOCKED
        //         lptr->status = LUNLOCKED;
        //         lptr->maxpriority = 0;
        //     }
        // }
        
    }

    // update the pinh of the current process
    llistnode_t *llcur = pptr->pllhead->next;
    while (llcur != pptr->plltail) {
        if (maxpriority < locktab[llcur->lock].maxpriority) maxpriority = locktab[llcur->lock].maxpriority;
        llcur = llcur->next;
    }

    if (pptr->pprio >= maxpriority) {
        pptr->pinh = 0;
    } else {
        pptr->pinh = maxpriority;
    }

    // pptr->pinh = maxpriority;

    restore(ps);
    return ret;
}

/* update the maximum scheduling priority of the lock */
void update_max_sched_priority(int lock) {
    int res = 0;

    qnode_t *cur = locktab[lock].qhead->next;
    qnode_t *end = locktab[lock].qtail;

    while (cur != end) {
        int curpriority = (proctab[cur->pid].pinh == 0)? proctab[cur->pid].pprio : proctab[cur->pid].pinh;
        if (curpriority > res) {
            res = curpriority;
        }
        cur = cur->next;
    }

    locktab[lock].maxpriority = res;
}

/* release a lock */
int release(int lock, int pid) {
    if (lock < 0 || lock >= NLOCKS) return SYSERR;

    if (locktab[lock].status != LLOCKED) return SYSERR;

    lock_t *lptr = &locktab[lock];

    plistnode_t *plcur = lptr->plhead->next;
    while (plcur != lptr->pltail) {
        if (plcur->pid = pid) break;
        plcur = plcur->next;
    }

    if (plcur == lptr->pltail) return SYSERR;

    // remove the lock from the lock list of the current process
    struct pentry *pptr = &proctab[pid];

    llistnode_t *llcur = pptr->pllhead->next;
    while (llcur != pptr->plltail) {
        if (llcur->lock == lock) break;
        llcur = llcur->next;
    }
    llcur->prev->next = llcur->next;
    llcur->next->prev = llcur->prev;
    freemem(llcur, sizeof(llistnode_t));

    // remove the current process from the proc list of the lock
    plcur->prev->next = plcur->next;
    plcur->next->prev = plcur->prev;
    freemem(plcur, sizeof(plistnode_t));

    // choose next process to hold the lock
    if (lptr->plhead->next == lptr->pltail) {
        qnode_t *qcur = lptr->qhead->next;

        if (qcur != lptr->qtail) {
            // set type
            lptr->type = qcur->type;

            if (qcur->type == READ) {
                int targetwaitprio = qcur->priority;
                while (qcur != lptr->qtail && qcur->priority == targetwaitprio && qcur->type == READ) {
                    // remove current process from the wait queue
                    qcur->prev->next = qcur->next;
                    qcur->next->prev = qcur->prev;

                    int nextpid = qcur->pid;
                    qnode_t *todelete = qcur;
                    qcur = qcur->next;
                    freemem(todelete, sizeof(qnode_t));

                    // insert the process into the proc list of the lock
                    insert_node_to_proc_list(lock, nextpid);
                    // insert the lock into the lock list of the process
                    insert_node_to_proc_lock_list(lock, nextpid);
                    proctab[nextpid].plock = -1;

                    // put the process into the ready queue
                    ready(nextpid, RESCHNO);
                }

                //inherit scheduling priority
                augment_priority(lock);
            } else {
                // remove the current process from the wait queue
                qcur->prev->next = qcur->next;
                qcur->next->prev = qcur->prev;
                int nextpid = qcur->pid;
                freemem(qcur, sizeof(qnode_t));

                // insert the process into the proc list of the lock
                insert_node_to_proc_list(lock, nextpid);
                // insert the lock to the lock list of the process
                insert_node_to_proc_lock_list(lock, nextpid);
                proctab[nextpid].plock = -1;

                // inherit scheduling priority
                augment_priority(lock);

                // put the process into the ready queue
                ready(nextpid, RESCHNO);
            }
        } else {
            // set the status of the lock to LUNLOCKED
            lptr->status = LUNLOCKED;
            lptr->maxpriority = 0;
        }
    }

    return OK;
}