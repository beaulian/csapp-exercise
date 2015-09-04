#include "csapp.h"

int mysystem(char *command)
{
    pid_t pid;
    int status;
    if (command == NULL)
        return -1;
    if ((pid = Fork()) < 0)
        return -1;

    else if (pid == 0){
        char *argv[] = {"sh","-c",command};
        execve("/bin/sh",argv,NULL);
        exit(-1); //子进程正常执行则不会执行此语句
    }

    while (1){
        if (waitpid(pid,&status,0) == -1){
            if (errno != EINTR)
                return -1;
        }
        else{
            if WIFEXITED(status)
                return WEXITSTATUS(status);
            else
                return status;
        }
    }

}

int main(int argc,char *argv[])
{
    int status;
    mysystem(argv[1]);
    exit(0);
}

