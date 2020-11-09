#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lock.h>


int lcreate() {
    STATWORD ps;
    disable(ps);

    int i;
    for (i = 0; i < NLOCKS; ++i) {
        if (++nextlock == NLOCKS) nextlock = 0;
        if (locktab[nextlock].status == LFREE) {
            locktab[nextlock].status = LUNLOCKED;
            // locktab[nextlock].owner = currpid;
            restore(ps);
            return nextlock;
        }
    }

    restore(ps);
    return SYSERR;
}