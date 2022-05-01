#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "userthreads.h"

typedef struct add_rows_args {
  int *a, *b, *res;
  int n;
} add_rows_args;

int add_rows(void *args)
{
  add_rows_args *mta = (add_rows_args *)args;
  for(int i=0; i<mta->n; i++) {
    mta->res[i] = mta->a[i] + mta->b[i];
  }
  exit();
}
// does clone system call to create threads and waits using join system 
// call for them to complete
// TEST: virtual address space (heap = mallocated data in this case and code)is shared
//       join system call blocks main thread

int test_shared_memory() {
  int n = 4;
  int **mat1 = (int**)(malloc(n*sizeof(int *)));
  int **mat2 = (int**)(malloc(n*sizeof(int *)));
  int **res = (int**)(malloc(n*sizeof(int *)));
  Thread *threads = (Thread*)(malloc(n*sizeof(Thread)));
  for(int i=0; i<n; i++) {
    res[i] = (int*)(malloc(n*sizeof(int)));
    mat1[i] = (int*)(malloc(n*sizeof(int)));  
    mat2[i] = (int*)(malloc(n*sizeof(int)));
  }
  for(int i=0; i<n; i++) {
    add_rows_args *mta = (add_rows_args*)(malloc(sizeof(add_rows_args)));
    mta->a = mat1[i];
    mta->b = mat2[i];
    mta->res = res[i];
    mta->n = n;
    thread_create(&threads[i], add_rows, mta);
  }

  for(int i=0; i<n; i++) {
    for(int j=0; j<n; j++) {
      if(res[i][j] != mat1[i][j] + mat2[i][j]){
        printf(1, "test matrix additon fail\n");
        return 0;
      }
    }
  }
  printf(1, "test matrix additon : PASS\n");
  return 0;
}

int c, c1, c2, run;
semaphore lock;
spinlock race_lock;
int race_cond_1(void *args)
{
  while (run == 1)
  {
    acquire_spinlock(&race_lock);
    c++;
    c1++;
    release_spinlock(&race_lock);
  }
  exit();
}
int race_cond_2(void *args)
{
  while (run == 1)
  {
    acquire_spinlock(&race_lock);
    c++;
    c2++;
    release_spinlock(&race_lock);
  }
  exit();
}

int test_race_condition() {
  c = 0;
  c1 = 0;
  c2 = 0;
  Thread t1, t2;
  run = 1;
  init_spinlock(&race_lock);
  thread_create(&t1, race_cond_1, 0);
  thread_create(&t2, race_cond_2, 0);
  sleep(5);
  run = 2;
  thread_join(&t1);
  thread_join(&t2);
  if (c != c1 + c2)
  {
    printf(1, "Race condition test fail!\n");
  }
  else {
    printf(1, "Race condition test pass, c = %d, c1 = %d, c2 = %d, c1+c2 = %d\n",c,c1,c2,c1+c2);
  }
  return 0;
}

int thread_create_func() {
  for(int i =0;i<1000;i++);
  exit();
}
int test_thread_creation() {
  Thread t1;
  int ret = thread_create(&t1,thread_create_func,0);
  thread_join(&t1);
  if(ret) {
    printf(1,"Thread create Test fail!\n");
  }
  else{
    printf(1,"Thread creation Test pass!\n");
  }
  return 0;
}

int thread_kill_func(void* args) {
  for(;;);
  exit();
}

// TEST: kill system call
int test_thread_kill() {
  Thread t1;
  thread_create(&t1,thread_kill_func,0);
  int ret = thread_kill(&t1);
  if(ret != 0) {
    printf(1,"Thread kill Test fail !\n");
  }
  else {
    printf(1,"Thread kill Test pass !\n");
  }
  return 0;
  
}

int bit, nflips;
spinlock bitlock;

int flipper(void *args) {
  int mybit = *(int *)args;
  for (int i = 0; i < 1000000; i++) {
    acquire_spinlock(&bitlock);
    if (bit != mybit) {
      bit = mybit;
      nflips += 1;
    }
    release_spinlock(&bitlock);
  }
  exit();
}

// TEST: test that threads work concurrently by checking how many times control switches between two threads
int test_concurrency() {
  nflips = 0;
  bit = 0;
  init_spinlock(&bitlock);
  Thread t1,t2;
  int mybit1 = 1, mybit2 = 2;
  thread_create(&t1, flipper, &mybit1);
  thread_create(&t2, flipper, &mybit2);
  thread_join(&t1);
  thread_join(&t2);
  //if serial execution took place and there wasn't concurrency, nflips would be exacty 2.
  //else it should be more than 2 in case of conccurent execution.
  if(nflips > 2) {
    printf(1, "concurrency test pass, bit flipped %d times\n", nflips);
  }
  else if(nflips == 2) {
    printf(1,"concurrency test fail\n");
  }
  else{
    printf(1,"unknown error in concurrency test\n");
  }
  return 0;
}

int test_join_tgl() {
  int ret = join(gettpid());
  if(ret != -1) {
    printf(1,"Join test fail !\n");
  }
  else {
    printf(1,"Join test pass!\n");
  }
  return 0;
}

// TEST: test that 40 threads can work simataneouly
int test_threads_stresstest() {
  Thread arr[40];
  int flag = 0;
  for(int i=0;i<40;i++) {
    if(thread_create(&arr[i],thread_create_func,0) != 0) {
      flag = 1;
    };
  }
  for(int i =0;i<40;i++) {
    thread_join(&arr[i]);
  }
  if(flag) {
    printf(1,"Stress thread test fail !\n");
  }
  else {
    printf(1,"Stress thread test pass !\n");
  }
  return 0;
  
}

int suspended;
int add_one(void *args)
{
  (*(int*)args)++;
  suspended = 0;
  kthread_suspend();
  (*(int*)args)++;
  exit();
}

// TEST: test suspend and resume.
int test_suspend_resume() {
  int x = 0;
  Thread t1;
  suspended = 0;
  thread_create(&t1, add_one, &x);
  sleep(5);
  // thread should have been suspended and value of x should be 1;
  // if not suspended it will be 2;
  if(x == 2) {
    printf(1,"suspend-resume test fail - error in suspend!\n");
    kthread_resume(t1.tid);
    return 0;
  }
  kthread_resume(t1.tid);
  thread_join(&t1);
  if(x==2) {
    printf(1,"suspend, resume test pass!\n");
  }
  else {
    printf(1,"suspend,resume test fail - error in resume! %d\n",x); 
  }
  return 0;
}

int test_wrong_syscall() {
  Thread t1;
  int ret = thread_create(&t1,0,0);
  if(ret != -1) {
    printf(1,"Wrong syscall test fail!\n");
  }
  else {
    printf(1,"Wrong syscall test pass!\n");
  }
  thread_join(&t1);
  return 0;
}

int main(int argc, char *argv[])
{
  test_race_condition();
  test_thread_creation();
  test_thread_kill();
  test_concurrency();
  test_shared_memory();
  test_join_tgl();
  test_threads_stresstest();
  test_suspend_resume();
  // test_wrong_syscall();
  exit();

}