/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);

	// the process is waiting in a lock
	if (pptr->plock != -1) {
		lock_t *lptr = &locktab[pptr->plock];
		// remove current process from the wait queue
		qnode_t *qptr = lptr->qhead->next;
		while (qptr != lptr->qtail) {
			if (qptr->pid == pid) {
				break;
			}
			qptr = qptr->next;
		}

		qptr->prev->next = qptr->next;
		qptr->next->prev = qptr->prev;

		freemem(qptr, sizeof(qnode_t));

		// ramp up the priority
		augment_priority(pptr->plock);
	}


	// the process is holding some locks
	if (pptr->pllhead->next != pptr->plltail) {
		llistnode_t *llptr = pptr->pllhead->next;
		while (llptr != pptr->plltail) {
			int lock = llptr->lock;
			llptr = llptr->next;
			release(lock, pid);
			// // remove current process from the proc list of the lock
			// lock_t *lptr = &locktab[llptr->lock];
			// plistnode_t *plptr = lptr->plhead->next;
			// while (plptr != lptr->pltail) {
			// 	if (plptr->pid = pid) break;
			// 	plptr = plptr->next;
			// }
			// plptr->prev->next = plptr->next;
			// plptr->next->prev = plptr->prev;
			// freemem(plptr, sizeof(plistnode_t));

			// llistnode_t *todelete = llptr;
			// llptr = llptr->next;
			// freemem(todelete, sizeof(llistnode_t));
		}
	}

	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}
