#include "defs.h"
#include "user.h"
#include "userthreads.h"

#define STACK_SIZE 4096

int thread_create(Thread *th, int (*fn)(void *),void* args) {
  th->state = NEW;
  th->stack = malloc(STACK_SIZE);
  if(th->stack == 0)
    return -1;
  th->tid = clone(fn, th->stack + STACK_SIZE, CLONE_FILES | CLONE_FS | CLONE_VM, args);
  if(th->tid == -1) {
      free(th->stack);
      th->stack = 0;
      printf(2, "Error while creating thread\n");
      th->state = ENDED;
      return -1;
  }
  th->state = RUNNING;
  return 0;
}

int thread_join(Thread *th) {
  int tid = join(th->tid);
  free(th->stack);
  th->stack = 0;
  th->state = ENDED;
  return tid;
}

int thread_kill(Thread *th) {
  if(th->state == ENDED) {
    return 0;
  }
  int ret = tkill(th->tid);
  free(th->stack);
  th->stack = 0;
  th->state = ENDED;
  return ret;
}