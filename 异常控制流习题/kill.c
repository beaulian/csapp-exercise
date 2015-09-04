#include "csapp.h"

int main()
{
    pid_t pid;
    //子进程休眠直到接收到一个信号
    if ((pid = Fork()) == 0){
        printf("child is %d\n",getpid());
        Pause();
        printf("control should never reach here\n");
        exit(0);
    }

    //父进程发送一个SIGKILL信号给子进程
    printf("parent is %d,%d\n",pid,getpid());
    Kill(pid,SIGKILL);  //子进程的pid
    exit(0);
}