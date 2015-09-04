#include "csapp.h"

unsigned int snooze(unsigned int secs)
{
    unsigned int rc = sleep(secs);
    printf("Slept for %d of %d secs.",secs-rc,secs);
    return rc;
}

void handler(int sig)
{
    return;  //当处理程序执行返回语句时,控制传递回控制流中进程被信号接收中断位置处的指令
}

int main(int argc,char **argv)
{
    if (argc != 2){
        fprintf(stderr,"usage: %s <secs>\n",argv[0]);
        exit(0);
    }
    if (signal(SIGINT, handler) == SIG_ERR)
        unix_error("signal error");

    (void)snooze(atoi(argv[1]));  //handler return后执行此函数

    exit(0);

}