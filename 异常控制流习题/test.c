#include "csapp.h"

int main()
{
    pid_t pid;
    if ((pid = fork()) == 0)
        printf("%d\n",getpid());  //父进程会输出两个,子进程则输出三个,格式是 a,a,b,b,a

    printf("%d\n",getpid());
    printf("%d\n",(int)getpgrp());
    exit(0);
}