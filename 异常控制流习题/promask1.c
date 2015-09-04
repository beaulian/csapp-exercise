#include "csapp.h"

void handler(int sig)
{
    pid_t pid;

    while ((pid = waitpid(-1,NULL,0)) > 0)
        deletejob(pid);
    if (errno != ECHILD)
        unix_error("waitpid error");
}

int main(int argc,char **argv)
{
    int pid;

    Signal(SIGCHLD,handler);
    initjobs();
    while (1){
        if ((pid = Fork()) == 0)
            Execve("/",argv,NULL);

        addjob(pid);
    }

    exit(0);
}