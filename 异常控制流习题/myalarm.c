#include "csapp.h"

void handler(int sig)
{
    static int beeps = 0;

    printf("BEEP\n");
    if (++beeps < 5)
        Alarm(1);  //接着主函数的Alarm()
    else {
        printf("BOOM!\n");
        exit(0);
    }
}

int main()
{
    Signal(SIGALRM,handler);   //信号处理程序:接收SIGALRM信号,然后异步调用handler函数
    Alarm(1);

    while (1);   //死循环
    exit(0);
}