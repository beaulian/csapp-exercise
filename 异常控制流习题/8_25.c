#include "csapp.h"

sigjmp_buf buf;

void handler(int sig)
{
    /*alarm()用来设置信号SIGALRM 在经过参数seconds 指定的秒数后传送给目前的进程
    . 如果参数seconds 为0, 则之前设置的闹钟会被取消, 并将剩下的时间返回.*/
    Alarm(0);   //将tfgets函数中的闹钟取消,释放资源
    siglongjmp(buf,1);
}

char *tfgets(char *string, int n, FILE *stream)
{
    Signal(SIGALRM,handler);

    Alarm(5);  //5秒后发送SIGALRM信号

    if (!sigsetjmp(buf,1))
        return Fgets(string,n,stream);
    else
        return NULL;
}

int main()
{
    char str[MAXLINE];
    memset(str,0,sizeof(str));
    if (tfgets(str,sizeof(str),stdin) != NULL)  //从屏幕中读取数据
        printf("read: %s\n",str);
    else
        printf("time out\n");

    exit(0);  //必须要
}
