#ifndef _LOCK_H_
#define _LOCK_H_


#define LFREE 0
#define LUNLOCKED 1
#define LLOCKED 2

#define READ 1
#define WRITE 2

#define NLOCKS 50

typedef struct _queuenode_t {
    int pid;                        // process id
    int type;                       // request type: READ, WRITE
    int priority;                   // waiting priority
    unsigned long timestamp;        // timestamp of the request: clktime
    struct _queuenode_t *next;      // next queue node
    struct _queuenode_t *prev;      // previous queue node
} qnode_t;

typedef struct _proc_list_node_t {
    int pid;
    struct _proc_list_node_t *prev;
    struct _proc_list_node_t *next;
} plistnode_t;

typedef struct _lock_list_node_t {
    int lock;
    struct _lock_list_node_t *prev;
    struct _lock_list_node_t *next;
} llistnode_t;

typedef struct _lock_t {
    int status;                 // lock state: LFREE, LUNLOCKED, LLOCKED
    int type;                   // lock type: READ, WRITE
    // int owner;                  // pid of the process which created the lock
    int maxpriority;            // maximum scheduling priority in the waiting queue
    plistnode_t *plhead;         // head of the list of processes holding the lock
    plistnode_t *pltail;         // tail of the list of processes holding the lock
    qnode_t *qhead;             // waiting queue head
    qnode_t *qtail;             // waiting queue tail
} lock_t;

lock_t locktab[NLOCKS];           // lock table
int nextlock;                   // next lock candidate

/* initialize the lock table */
void linit();
/* create a lock */
int lcreate();
/* destroy a lock */
int ldelete(int);
/* acquire a lock for reads or writes */
int lock(int, int, int);
/* release multiple locks */
int releaseall(int, long);

void insert_node_to_proc_list(int, int);
void insert_node_to_proc_lock_list(int, int);
int get_max_write_priority(int);
void insert_node_to_wait_queue(int, int, int, int);
void augment_priority(int);
void update_max_sched_priority(int);
int release(int, int);

#endif