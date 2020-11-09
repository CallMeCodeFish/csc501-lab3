#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <sleep.h>
#include <stdio.h>
#include <lock.h>


int lock(int lock, int type, int priority) {
    STATWORD ps;
    disable(ps);

    // validate lock id
    if (lock < 0 || lock >= NLOCKS) {
        restore(ps);
        return SYSERR;
    }

    // validate type
    if (type != READ && type != WRITE) {
        restore(ps);
        return SYSERR;
    }

    // validate lock status
    if (locktab[lock].status == LFREE) {
        restore(ps);
        return SYSERR;
    }

    // validate the scenario that the lock was once deleted
    if (proctab[currpid].plockcallback[lock] == -1) {
        restore(ps);
        return SYSERR;
    }

    // the first time the current process call lock() ??
    if (proctab[currpid].plockcallback[lock] == 0) {
        proctab[currpid].plockcallback[lock] = 1;
    }

    if (locktab[lock].status == LUNLOCKED) {
        // acquire the lock directly
        locktab[lock].status = LLOCKED;
        locktab[lock].type = type;
        insert_node_to_proc_list(lock, currpid);
        insert_node_to_proc_lock_list(lock, currpid);
        restore(ps);
        return OK;
    } else {
        // the lock is held by another process
        int maxwriteprio = get_max_write_priority(lock);
        if (type == READ && locktab[lock].type == READ && priority >= maxwriteprio) {
            insert_node_to_proc_list(lock, currpid);
            insert_node_to_proc_lock_list(lock, currpid);
            restore(ps);
            return OK;
        } else {
            struct pentry *pptr;
            pptr = &proctab[currpid];

            // need to wait
            insert_node_to_wait_queue(lock, currpid, type, priority);

            // invert scheduling priority
            augment_priority(lock);

            pptr->pstate = PRWAIT;
            pptr->plock = lock;
            pptr->plockret = OK;
            resched();

            restore(ps);
            return pptr->plockret;
        }
    }
}

/* insert a process to the holding list of a lock */
void insert_node_to_proc_list(int lock, int pid) {
    plistnode_t *newnode = getmem(sizeof(plistnode_t));
    newnode->pid = pid;
    newnode->prev = locktab[lock].pltail->prev;
    newnode->prev->next = newnode;
    newnode->next = locktab[lock].pltail;
    locktab[lock].pltail->prev = newnode;
}

/* insert a lock to the holding lock list of a process */
void insert_node_to_proc_lock_list(int lock, int pid) {
    llistnode_t *newnode = getmem(sizeof(llistnode_t));
    newnode->lock = lock;
    newnode->prev =  proctab[pid].plltail->prev;
    newnode->prev->next = newnode;
    newnode->next = proctab[pid].plltail;
    proctab[pid].plltail->prev = newnode;
}

/* calculate the maximum writer wait priority in the wait queue of a lock */
int get_max_write_priority(int lock) {
    int res = -214783648;

    qnode_t *cur = locktab[lock].qhead->next;
    qnode_t *end = locktab[lock].qtail;

    while (cur != end) {
        if (cur->type == WRITE && cur->priority > res) {
            res = cur->priority;
        }
        cur = cur->next;
    }

    return res;
}

/* insert a process into the wait queue of a lock */
void insert_node_to_wait_queue(int lock, int pid, int type, int priority) {
    qnode_t *newnode = getmem(sizeof(qnode_t));
    newnode->pid = pid;
    newnode->type = type;
    newnode->priority = priority;
    newnode->timestamp = (type == READ)? clktime + 2 : clktime;

    qnode_t *cur = locktab[lock].qhead;
    qnode_t *end = locktab[lock].qtail;
    qnode_t *next;
    while (cur->next != end) {
        next = cur->next;
        if (priority >= next->priority) {
            break;
        }
        cur = cur->next;
    }

    if (cur->next == end || cur->next->priority < priority) {
        newnode->next = cur->next;
        newnode->next->prev = newnode;
        newnode->prev = cur;
        cur->next = newnode;
    } else {
        while (cur->next->priority == priority) {
            next = cur->next;
            if (newnode->timestamp < next->timestamp) {
                break;
            }
            cur = cur->next;
        }
        newnode->next = cur->next;
        newnode->next->prev = newnode;
        newnode->prev = cur;
        cur->next = newnode;
    }
}

/* invert scheduling priority of the processes holding the lock */
void augment_priority(int lock) {
    // update the maximum priority of the lock
    update_max_sched_priority(lock);

    lock_t *lptr;
    lptr = &locktab[lock];

    plistnode_t *cur = lptr->plhead->next;
    plistnode_t *end = lptr->pltail;

    while (cur != end) {
        struct pentry *pptr = &proctab[cur->pid];
        if (pptr->pinh == 0) {
            if (pptr->pprio < lptr->maxpriority) {
                pptr->pinh = lptr->maxpriority;
                if (pptr->plock != -1) {
                    augment_priority(pptr->plock);
                }
            }
        } else {
            if (pptr->pprio < lptr->maxpriority) {
                pptr->pinh = lptr->maxpriority;
            } else {
                pptr->pinh = 0;
            }

            if (pptr->plock != -1) {
                augment_priority(pptr->plock);
            }
        }
        cur = cur->next;
    }
}