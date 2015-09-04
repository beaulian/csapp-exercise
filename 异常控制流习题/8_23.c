#include "csapp.h"

int counter = 0;

void handler(int sig)
{
    counter++;
    sleep(1);   //Do some work in the handler
    return;
}

int main()
{
    int i;
    Signal(SIGUSR2,handler);

    if (Fork() == 0){
        for (i=0; i<5; i++){
            Kill(getppid(),SIGUSR2);
            printf("sent SIGUSR2 to parent\n");
            /*加上sleep(1)就正确,因为主函数与信号处理函数并发执行,连续发送同一个信号,
            有可能信号被阻塞,然后被丢弃,counter就会比实际的小*/
            sleep(1);
        }
        exit(0);
    }

    Wait(NULL);  //等待子进程终止
    printf("counter=%d\n",counter);
    exit(0);
}