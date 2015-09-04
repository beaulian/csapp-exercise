#include "csapp.h"

void handler(int sig)
{
    printf("\nCaught SIGINT\n");
    exit(0);
}

int main()
{
    //安装SIGINT信号处理程序
    if (signal(SIGINT, handler) == SIG_ERR){
        unix_error("signal error");
    }

    pause();   //接收一个信号,这里是接收来自键盘的信号(ctrl-c),然后调用signal函数,如果没有此函数进程就立刻终止
    exit(0);
}