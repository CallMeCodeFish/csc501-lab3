#include <kernel.h>
#include <lock.h>

void linit() {
    nextlock = -1;

    int i;
    for (i = 0; i < NLOCKS; ++i) {
        locktab[i].status = LFREE;
        locktab[i].type = 0;
        // locktab[i].owner = -1;
        locktab[i].maxpriority = 0;
        
        locktab[i].plhead = getmem(sizeof(plistnode_t));
        locktab[i].pltail = getmem(sizeof(plistnode_t));

        locktab[i].plhead->pid = -1;
        locktab[i].plhead->prev = NULL;
        locktab[i].plhead->next = locktab[i].pltail;

        locktab[i].pltail->pid = -1;
        locktab[i].pltail->prev = locktab[i].plhead;
        locktab[i].pltail->next = NULL;

        locktab[i].qhead = getmem(sizeof(qnode_t));
        locktab[i].qtail = getmem(sizeof(qnode_t));

        locktab[i].qhead->pid = -1;
        locktab[i].qhead->type = 0;
        locktab[i].qhead->priority = 0;
        locktab[i].qhead->timestamp = 0;
        locktab[i].qhead->prev = NULL;
        locktab[i].qhead->next = locktab[i].qtail;

        locktab[i].qtail->pid = -1;
        locktab[i].qtail->type = 0;
        locktab[i].qtail->priority = 0;
        locktab[i].qtail->timestamp = 0;
        locktab[i].qtail->prev = locktab[i].qhead;
        locktab[i].qtail->next = NULL;
    }
}