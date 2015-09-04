#include "csapp.h"
#define N 2

int main()
{
    int status,i;
    //FILE *fp;
    //fp = fopen("8_23.c","r");
    int *ptr = NULL;
    pid_t pid;

    for (i=0; i<N; i++){
        if ((pid = Fork()) == 0){
            //fputc('s',fp);
            //fclose(fp);
            *ptr= 0;  //访问不存在的内存地址
            exit(0);
        }
    }
    //即使是按照顺序fork的子进程,在最后printf的时候也不能确定顺序
    while ((pid = waitpid(-1,&status,0)) > 0){
        if (WIFEXITED(status))
            printf("child %d terminated normally with exit status=%d\n",pid,WEXITSTATUS(status));
        else
            printf("child %d terminated by signal %d: Segmentation fault\n",pid,WTERMSIG(status));
    }
    /*回收所有子进程后,再调用waitpid就返回-1,并且设置全局变量errno为ECHILD*/
    if (errno != ECHILD)
        unix_error("waitpid error");

    exit(0);
}