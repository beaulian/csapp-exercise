#include "csapp.h"
#define MAXARGS 128

void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

static int counter = 0;  //作业ID(作业编号)
pid_t pid;

void handler1(int sig)
{
    printf("Job %d terminated by signal: Interrupt\n",pid);
    return;
}

void handler2(int sig)
{
    printf("Job [%d] %d terminated by signal: Stopped\n",counter,pid);
    return;
}

void handler3(int sig)
{
    printf("Job %d terminated by signal: Terminated\n",pid);
    return;
}

int main()
{
    char cmdline[MAXLINE];
    while (1){
        printf(">");
        Fgets(cmdline,MAXLINE,stdin);
        if (feof(stdin))
            exit(0);
        eval(cmdline);
    }
}

void eval(char *cmdline)
{
    char *argv[MAXARGS];
    char buf[MAXLINE];  //因为要修改命令行参数,所以留一个拷贝,把buf改了
    int bg;   //判断是不是在后台运行,如果是bg等于1,否则为0


    strcpy(buf,cmdline);
    bg = parseline(buf,argv);
    if (argv[0] == NULL)
        return;
    if (!builtin_command(argv)){
        if ((pid = Fork()) == 0){  //生成子进程(作业是进程组,这里的作业只有一个子进程)
            if (execve(argv[0],argv,environ) < 0){  //调用execve函数,加载运行新的进程
                printf("%s: Command not found\n",argv[0]);
                exit(0);     //一个子进程(整个作业)退出,就是再次出现一个">"
            }
        }
        //父进程
        Signal(SIGINT,handler1);
        Signal(SIGTSTP,handler2);  //关于为什么要和waitpid一起用,还是不明白
        Signal(SIGKILL,handler3);

        if (!bg){   //当bg为1时就是后台子进程,即脱离父进程,为0时,外壳会等待它完成
            int status;
            if (waitpid(pid,&status,0) < 0)
                unix_error("waitfg: waitpid error\n");
        }
        else
            printf("[%d] %d %s\n", ++counter, pid,argv[0]);
    }

    return;
}

int parseline(char *buf, char **argv)
{
    char *delim;   //指向分隔符的指针
    int argc;
    int bg;

    buf[strlen(buf)-1] = ' '; //把'\n'换成空格
    while (*buf && (*buf == ' '))
        buf++;     //忽略开头的空格;

    //建立argv列表
    argc = 0;
    while ((delim = strchr(buf,' '))){   //查找字符串中首次出现某个字符的位置
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;  //指针像前移
        while (*buf && (*buf == ' '))
            buf++;
    }
    argv[argc] = NULL;

    if (argc == 0)
        return 1;   //忽略空行

    if ((bg = (*argv[argc-1] == '&')) != 0)  //如果存在&就去掉argv里的&
        argv[--argc] = NULL;

    return bg;
}

int builtin_command(char **argv)   //这里的argv来源于parseline函数
{
    if (!strcmp(argv[0],"quit"))  ////exit 命令
        exit(0);
    if (!strcmp(argv[0],"&"))  //如果只是单独的&,则忽略
        return 1;
    return 0;
}

