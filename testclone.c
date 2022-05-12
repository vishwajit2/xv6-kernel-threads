#include "types.h"
#include "user.h"
#include "fcntl.h"

int demo(void *args)
{
    printf(1,"In demo !%d\n",*(int*)args);
    printf(1,"HEllo");
    *(int*)args += 100;
    for(;;);
    exit();
}

int main(int argc,char *argv[]) {
    int a = 100;
    printf(1,"Initial Value : %d\n",a);
    char* s1 = malloc(4096);
    char* s2 = malloc(4096);
    char* s3 = malloc(4096);
    s1 = s1 + 4096;
    s2 = s2 + 4096;
    s3 = s3 + 4096;
    int p1 = clone(demo,(void *)s1,14,&a);
    int p2 = clone(demo,(void *)s2,14,&a);
    int p3 = clone(demo,(void *)s3,14,&a);
    printf(1,"Thread PIDs : %d %d %d\n",p1,p2,p3);
    tgkill();
    printf(1,"Changed Value : %d\n",a);
    exit();
}