/*守护进程编写*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<fcntl.h>

int main()
{
    pid_t pid;
    int i,fd;
    char *buf = "This is a daemon program";
    //创建子进程
    if ((pid = fork()) < 0){
        printf("fork error!");
        exit(1);
    }else if (pid > 0){
        exit(0);  //父进程退出,因为父进程fork之后返回的值大于0
    }

    setsid();  //在子进程中创建新会话
    chdir("/");  //设置工作目录为根
    umask(0);  //设置权限掩码

    for (i=0; i<getdtablesize(); i++)  ////getdtablesize返回子进程文件描述符表的项数
        close(i);  //关闭这些不用的文件描述符

    for (i=0;i<10;i++){
        if ((fd = open("/home/gavin/Desktop/daemon.log",O_CREAT|O_WRONLY|O_APPEND,0600)) < 0){
            printf("Open file error\n");
            exit(1);
        }

        //将buf写到fd中
        write(fd,buf,strlen(buf)+1);
        close(fd);
        sleep(10);
        printf("Never output\n");
    }

    return 0;
}