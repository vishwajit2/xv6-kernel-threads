#ifndef THREAD_H
#define THREAD_H 1
#define CLONE_PARENT 1
#define CLONE_FILES  2
#define CLONE_FS     4
#define CLONE_VM     8

#define NEW       0
#define RUNNING   1
#define KILLED    2
#define SUSPENDED 3
#define ENDED     4 

typedef struct Thread {
    int tid;
    int state;
    char* stack;
} Thread;

#define QUEUE_SIZE 10

typedef struct queue {
    int front;
    int rear;
    int data[QUEUE_SIZE];
} queue;

typedef struct spinlock {
    uint locked;
    struct cpu *cpu;
} spinlock;

typedef struct semaphore {
    int value;
    queue list;
    spinlock s;
} semaphore;

int thread_create(Thread *th, int (*fn)(void *), void *args);
int thread_join(Thread *th);
int thread_kill(Thread *th);
int semaphore_wait(semaphore *s);
int semaphore_signal(semaphore *s);
void init_semaphore(semaphore *s, int value);

#endif