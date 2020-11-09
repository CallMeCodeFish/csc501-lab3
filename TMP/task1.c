#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <stdio.h>
#include <lock.h>

#define DEFAULT_LOCK_PRIO 20

void lock_writer(char *msg, int lck) {
    kprintf("\t\t%s: to acquire lock\n", msg);
    lock(lck, WRITE, DEFAULT_LOCK_PRIO);
    kprintf("\t\t%s: acquired lock, enter infinite loop\n", msg);
    while (1) {
        ;
    }
    kprintf("\t\t%s: to release lock\n", msg);
    releaseall(1, lck);
}

void lock_reader(char *msg, int lck) {
    kprintf("\t\t%s: to acquire lock\n", msg);
    lock(lck, READ, DEFAULT_LOCK_PRIO);
    kprintf("\t\t%s: acquired lock\n", msg);
    kprintf("\t\t%s: to release lock\n", msg);
    releaseall(1, lck);
}

void sem_proc1(char *msg, int sem) {
    kprintf("\t\t%s: to acquire semaphore\n", msg);
    wait(sem);
    kprintf("\t\t%s: acquired semaphore, enter infinite loop\n", msg);
    while (1) {
        ;
    }
    kprintf("\t\t%s: to release semaphore\n", msg);
    signal(sem);
}

void sem_proc2(char *msg, int sem) {
    kprintf("\t\t%s: to acquire semaphore\n", msg);
    wait(sem);
    kprintf("\t\t%s: acquired semaphore\n", msg);
    kprintf("\t\t%s: to release semaphore\n", msg);
    signal(sem);
}

void mytest() {
    int lck, sem;
    kprintf("\nMy test\n");
    kprintf("\nresult of using locks:\n");
    lck = lcreate();

    int pid1 = create(lock_writer, 2000, 20, "lock_writer", 2, "A", lck);
    int pid2 = create(lock_reader, 2000, 30, "lock_reader", 2, "B", lck);

    kprintf("\tstart A (priority: 20), then sleep 1s\n");
    resume(pid1);
    sleep(1);
    kprintf("\tcurrent priority of A: %d\n", getprio(pid1));

    kprintf("\tstart B (priority: 30), then sleep 1s\n");
    resume(pid2);
    sleep(1);
    kprintf("\tcurrent priority of A: %d\n", getprio(pid1));

    kill(pid2);
    kill(pid1);

    kprintf("\nresult of using semaphores:\n");
    sem = screate(1);

    int pid3 = create(sem_proc1, 2000, 20, "sem_proc1", 2, "A", sem);
    int pid4 = create(sem_proc2, 2000, 30, "sem_proc2", 2, "B", sem);

    kprintf("\tstart A (priority: 20), then sleep 1s\n");
    resume(pid3);
    sleep(1);
    kprintf("\tcurrent priority of A: %d\n", getprio(pid3));

    kprintf("\tstart B (priority: 30), then sleep 1s\n");
    resume(pid4);
    sleep(1);
    kprintf("\tcurrent priority of A: %d\n", getprio(pid3));

    kill(pid4);
    kill(pid3);
}