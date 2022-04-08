#include "types.h"
#include "user.h"

int demo(void* args) {
    printf(0,"HEllo !%d\n",*(int*)args);
    *(int*)args += 100;
    exit();
}

int main(int argc,char *argv[]) {
    int a = 100;
    printf(0,"Initial Value : %d\n",a);
    char* stack = sbrk(4096);
    stack = stack + 4096;
    int pid = clone(demo,(void *)stack,10,&a);
    printf(0,"Thread PID : %d\n",pid);
    // join(pid);
    printf(0,"From main function!, pid : %d\n",pid);
    printf(0,"Changed Value : %d\n",a);
    exit();
}