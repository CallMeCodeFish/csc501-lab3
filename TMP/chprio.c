/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	int oldprio = pptr->pprio;
	pptr->pprio = newprio;

	if (oldprio != newprio) {
		if (pptr->pinh == 0 && pptr->plock != -1) {
			augment_priority(pptr->plock);
		} else if (pptr->pinh != 0 && pptr->pinh <= newprio) {
			int oldpinh = pptr->pinh;
			pptr->pinh = 0;
			if (oldpinh < newprio && pptr->plock != -1) {
				augment_priority(pptr->plock);                         
			}
		}
	}

	restore(ps);
	return(newprio);
}
