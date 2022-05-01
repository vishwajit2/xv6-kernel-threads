#include "types.h"
#include "fcntl.h"
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

static uint xchg(volatile uint *addr, uint newval)
{
  uint result;

  // The + in "+m" denotes a read-modify-write operand.
  asm volatile("lock; xchgl %0, %1"
               : "+m"(*addr), "=a"(result)
               : "1"(newval)
               : "cc");
  return result;
}

void acquire_spinlock(spinlock *s)
{
  while (xchg(&(s->locked), 1) != 0)
    ;
  __sync_synchronize();
}

void init_spinlock(spinlock *s)
{
  s->locked = 0;
  s->cpu = 0;
}

void release_spinlock(spinlock *s)
{
  s->cpu = 0;
  __sync_synchronize();
  asm volatile("movl $0, %0"
               : "+m"(s->locked)
               :);
}

int init_queue(queue *q)
{
  if (!q)
    return 0;
  q->front = q->rear = -1;
  return 0;
}

int is_queue_full(queue q)
{
  return (q.front == (q.rear + 1) % QUEUE_SIZE);
}

int is_queue_empty(queue q)
{
  return (q.front == -1);
}

int enqueue(queue *q, int x)
{
  if (is_queue_full(*q) == 1)
    return -1;
  if (q->front == -1)
    q->front = 0;
  q->rear = (q->rear + 1) % QUEUE_SIZE;
  q->data[q->rear] = x;
  return 0;
}

int dequeue(queue *q)
{
  int ret;
  if (is_queue_empty(*q) == 1)
    return -1;
  ret = q->data[q->front];
  if (q->front == q->rear)
  {
    q->front = q->rear = -1;
  }
  else
  {
    q->front = (q->front + 1) % QUEUE_SIZE;
  }
  return ret;
}

void init_semaphore(semaphore *s, int value)
{
  s->value = value;
  init_spinlock(&s->s);
  init_queue(&s->list);
}

int semaphore_wait(semaphore *sem)
{
  acquire_spinlock(&(sem->s));
  printf(1, "pid %d : called wait\n", gettpid());
  while (sem->value <= 0)
  {
    printf(1,"SEM VALUE : %d\n",sem->value);
    int t = gettpid();
    enqueue(&sem->list, t);
    printf(1, "pid %d : value before suspend : %d\n", t, sem->value);
    release_spinlock(&(sem->s));
    kthread_suspend();
    acquire_spinlock(&(sem->s));
    printf(1, "pid %d : value after suspend : %d\n", t, sem->value);
  }
  sem->value--;
  printf(1, "pid %d : out of kthread_suspend!\n", gettpid());
  release_spinlock(&(sem->s));
  return 0;
}

int semaphore_signal(semaphore *sem)
{
  acquire_spinlock(&(sem->s));
  sem->value++;
  if (!is_queue_empty(sem->list))
  {
    int x = dequeue(&(sem->list));
    printf(1, "pid %d : TRESUME on %d\n", gettpid(), x);
    release_spinlock(&(sem->s));
    kthread_resume(x);
    return 0;
  }
  release_spinlock(&(sem->s));
  return 0;
}