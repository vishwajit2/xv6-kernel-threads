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
#endif