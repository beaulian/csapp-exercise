/*
 * unix shell with job control
 *内建命令是fg,bg,jobs,echo,quit,&
 * @copyright 官加文
 */

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<ctype.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<errno.h>

/* 宏定义一些常数 */
#define MAXLINE     1024   //最大的行大小
#define MAXARGS     128    //最多的参数
#define MAXJOBS     16     //在任何时间内最多的作业
#define MAXJID      1<<16  //最大的作业ID,为65536

/*
 * 1<<16
 * 左移16位
 * 0000000000000001(2)==1
 * 1000000000000000(2)==2^16==65536
 */

/* 定义作业的状态  */
#define UNDEF 0   //undefined
#define FG    1   //在前台运行
#define BG    2   //在后台运行
#define ST    3   //stopped

/*
 * 作业状态: FG(foreground), BG(background), ST(stopped)
 * 作业状态变化说明(开始状态 -> 变化状态):
 *    FG -> ST  : ctrl-z
 *    ST -> FG  : fg command
 *    ST -> BG  : bg command
 *    BG -> FG  : fg command
 *    没有FG -> BG的状态,至多一个作业运行在前台
 */

/* 全局变量 */
/* c标准库定义了一个全局变量environ, 保存着环境变量
 * extern引入外部文件的全局变量
 */
extern char **environ;
char prompt[] = "tsh>";  //命令提示符
int verbose = 0;         //如果为真,打印额外的输出
int nextjid = 1;         //分配下一个作业ID[1,2,3....]
char sbuf[MAXLINE];      //for composing sprintf(把格式化的数据写入某个字符串) messages

struct job_t {
    pid_t pid;              //作业的进程组ID
    int jid;                //作业ID,即作业标识符 [1,2, ...]
    int state;              //UNDEF, BG, FG, or ST
    char cmdline[MAXLINE];  //命令行
};

struct job_t jobs[MAXJOBS];   //作业列表


/* 函数原型 */
void eval(char *cmdline);       //解析命令行
int builtin_cmd(char **argv);   //判断是否是内建命令
void do_bgfg(char **argv);      //实现bg,fg两个内建命令
void do_echo(char **argv);
void waitfg(pid_t pid);         //等待前台进程终止


/* 信号处理函数 */
void sigchld_handler(int sig);   //SIGCHLD
void sigtstp_handler(int sig);   //SIGTSTP
void sigint_handler(int sig);    //SIGINT
void sigquit_handler(int sig);   //SIGQUIT,进程在退出时会产生core文件,(相当于程序错误信号)

void mask_sigchld_signal(int how, int sig);

int parseline(const char *cmdline, char **argv);

/* 关于作业控制的函数 */
void clear_job(struct job_t *job);
void init_jobs(struct job_t *jobs);
int max_jid(struct job_t *jobs);
int add_job(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int delete_job(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);  //前台进程组长ID
struct job_t *get_job_pid(struct job_t *jobs, pid_t pid);
struct job_t *get_job_jid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void list_jobs(struct job_t *jobs);

void usage(void);   //usage是用法的意思(for -h命令行参数)
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);   //将void定义为handler_t
handler_t *Signal(int signum, handler_t *handler);
void Kill(pid_t pid, int signum);


/* 主程序(main routine) */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1;  //默认输出命令提示符

    /* 重定向标准错误流(stderr 文件描述符为2)
     * 到标准输出流(stdout 文件描述符为1)
     * 这样驱动就能得到所有输出在被连接到stdout的管道上
     * dup2(oldfd,newfd),被指向oldfd
     */
    dup2(1, 2);

    /* 解析(parse)命令行参数 */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
            case 'h':
                usage();
                break;
            case 'v':
                verbose = 1;   //输出额外的诊断信息,如果命令行参数有-v时则verbose置为1
                break;
            case 'p':
                emit_prompt = 0;
                break;
            default:
                usage();  //如果参数有误,默认参数为-h
        }
    }

    /* 安装信号处理程序 */
    Signal(SIGINT, sigint_handler);    //ctrl-c,signum为2
    Signal(SIGTSTP, sigtstp_handler);  //ctrl-z,signum为20
    Signal(SIGCHLD, sigchld_handler);  //子进程终止或停止,比如SIGKILL杀死,signum为17

    /* 和SIGINT类似, 但由QUIT字符(通常是Ctrl-\)来控制.
     * 进程在因收到SIGQUIT退出时会产生core文件,
     * 在这个意义上类似于一个程序错误信号,
     * 提供一个干净的方式杀死shell
     */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* 初始化作业列表 */
    init_jobs(jobs);

    /* 读取命令行 */
    while (1) {
        if (emit_prompt) {
            printf("%s ", prompt);
            fflush(stdout);   //清除缓冲区,为了立刻看到输出的东西
        }
        /*  ferror返回0表示未出错,返回非零表示出错 */
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error\n");
        if (feof(stdin)) {    //ctrl-d 是文件结束
            fflush(stdout);   //清空缓冲区
            exit(0);
        }

        /* 求命令行的值 */
        eval(cmdline);
        fflush(stdout);
    }

    exit(0);   /* 控制永远不会到达这里 */
}

void eval(char *cmdline)
{
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    pid_t pid;
    strcpy(buf, cmdline);
    bg = parseline(cmdline, argv);
    if (argv[0] == NULL)
        return;
    if (!builtin_cmd(argv)){
        /* 解决经典父进程和子进程的竞争问题:
         * 在调用fork之前阻塞SIGCHLD,保证在子
         * 进程被添加到作业列表之后回收该子进程
         * 即处理add_job与子进程结束的竞争条件
         */
        mask_sigchld_signal(SIG_BLOCK, SIGCHLD);   //阻塞SIGCHLD信号
        if ((pid = fork()) == 0){
            //解除阻塞,因为子进程继承了父进程被阻塞信号的集合
            mask_sigchld_signal(SIG_UNBLOCK, SIGCHLD);
            /* After the fork, but before the execve,
             * the child process should call
             * setpgid(0, 0), which puts the child in
             * a new process group whose group ID is
             * identical to the child’s PID. This
             * ensures that there will be only one
             * process, your shell, in the
             * foreground process group.
            */
            if (setpgid(0, 0) < 0) { /* put the child in a new process group */
                unix_error("eval: setpgid failed");
            }
            if (execve(argv[0], argv, environ) < 0){
                printf("%s: command not found\n", argv[0]);
                exit(0);
            }

        }
        else {
           //父进程
            int status;
            if (!bg) {
                add_job(jobs, pid, FG, cmdline);
                //printf("success add FG process\n");
            }
            else {
                add_job(jobs, pid, BG, cmdline);
                //printf("success add BG process\n");
            }

            /* add_job之后解除阻塞 */
            mask_sigchld_signal(SIG_UNBLOCK, SIGCHLD);
            if (!bg) {
                 /* 等待前台进程终止 */
                waitfg(pid);
            }else {
                printf("[%d]+ (%d) %s", pid2jid(pid), pid, cmdline);
            }
        }

    }

}

int parseline(const char *cmdline, char **argv)
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') { //*buf是单个字符
        buf++;
        delim = strchr(buf, '\'');
    }
    else {
	    delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf; /* 以ls -l为例,此处buf是"ls-l",argv[0]为"ls -l"*/
	*delim = '\0';  /* delim从" -l"变为 " ", buf 变为"ls",argv[0]变为"" */
	buf = delim + 1; /* buf 变为 "-l" */
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;

    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

int builtin_cmd(char **argv)
{
    if (!strcmp("quit",argv[0]))
        exit(0);
    if (!strcmp("&",argv[0]))
        return 1;

    /* 增加jobs命令 */
    if (!strcmp("jobs", argv[0])) {
        list_jobs(jobs);
        return 1;
    }

    /* 增加echo命令 */
    if (!strcmp("echo", argv[0])) {
        do_echo(argv);
        return 1;
    }

    /* 增加fg,bg命令 */
    if (!strcmp(argv[0],"fg") || !strcmp(argv[0], "bg")) {
        //printf("bg/fg\n");
        do_bgfg(argv);
        return 1;
    }
    return 0;
}

/* 执行内建的fg和bg命令 */
void do_bgfg(char **argv)
{
    int i, jobid;
    char *id = argv[1];
    struct job_t *job;
    if (id == NULL) {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return; //退出函数
    }

    if (id[0] == '%') {
        jobid = atoi(++id);   //是++id,不是id++
        if ((job = get_job_jid(jobs, jobid)) == NULL) {
            printf("%s: No such job\n", id);
            return;
        }
    }

    else {
        if (atoi(id) != 0) {
            if ((job = get_job_pid(jobs, atoi(id))) == NULL) {
                printf("(%s): No such process\n", id);
                return;
            }

        }
    }

    if (!strcmp(argv[0], "fg")) {
        job->state = FG;
        //唤醒job,SIGCONT让终止的进程继续,signum为18
        Kill(-1 * job->pid, SIGCONT);  //乘-1是要发送给整个进程组的意思
        waitfg(job->pid);
    }

    if (!strcmp(argv[0], "bg")) {
        job->state = BG;
        //唤醒job,SIGCONT让终止的进程继续
        Kill(-1 * job->pid, SIGCONT);  //乘-1是要发送给整个进程组的意思
    }

    return;
}

void do_echo(char **argv)
{
    int i;
    for (i=1; argv[i] != NULL; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
}

//一直阻塞,直到不再为前台进程组
void waitfg(pid_t pid)
{
    while (pid == fgpid(jobs)) { //死循环监听
        sleep(0);
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 * a child job terminates (becomes a zombie), or stops because it
 * received a SIGSTOP or SIGTSTP signal. The handler reaps all
 * available zombie(僵尸) children, but doesn't wait for any other
 * currently running children to terminate.
 */
/* 子进程成为僵尸进程的方法有很多种*/
void sigchld_handler(int sig)
{
    int status, child_sig;
    pid_t pid;
    struct job_t *job;
    //WUNTRACED | WNOHANG很重要,不要傻等,不然遇到直接cat的命令会一直阻塞
    while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) {
        //printf("Handler child %d\n", (int) pid);
        if (WIFSTOPPED(status)) {  //子进程处理停止信号(这里假设所有的停止信号都是SIGTSTP,即ctrl-z)
            sigtstp_handler(WSTOPSIG(status));  //直接cat命令获得信号21(后台进程从终端读),但是我也不知道怎么来的
        }
        else if (WIFSIGNALED(status)) {   //子进程因为未捕获的信号而停止
            //printf("yes\n");
            child_sig = WTERMSIG(status);
            if (child_sig == SIGINT)
                sigint_handler(child_sig);
            else
                unix_error("sigchld_handler: unknown signal\n");
        }
        else {
            //printf("yes\n");
            //后台作业也应该被删除
            delete_job(jobs, pid);   //如果正常退出
        }
    }
    return;
}

/* SIGINT和SIGTSTP都是针对于前台进程组,所以要获得前台进程组的 */
void sigint_handler(int sig)
{
    //printf("terminated sig\n");
    pid_t pid;

    pid = fgpid(jobs);   //获得前台进程ID,不考虑管道,即一个作业只有一个进程
    int jid = pid2jid(pid);
    if (pid != 0) {      //如果有前台进程则不返回0
        printf("Job [%d] (%d) terminated by signal %d", jid, pid, sig);
        delete_job(jobs,pid);   /* 顺序不能变 */
        Kill(-pid, sig);   //发送SIGINT信号
    }

    return;
}

void sigtstp_handler(int sig)
{
    //printf("stop sig\n");
    pid_t pid;
    pid = fgpid(jobs);
    int jid = pid2jid(pid);
    struct job_t *job;

    if (pid != 0) {
        printf("Job [%d] (%d) stopped by signal %d\n", jid, pid, sig);
        job = get_job_pid(jobs, pid);
        job->state=ST;
        Kill(-pid, sig);   //发送SIGTSTP信号
    }
}

void sigquit_handler(int sig)
{
    printf("Terminating after receipt of SIGQUIT signal(core dumped)\n");   //receipt是收到的意思
    exit(1);
}
/* 信号处理函数结束 */

void mask_sigchld_signal(int how, int sig)
{
    sigset_t signals;  //信号集
    if (sigemptyset(&signals) < 0)
        unix_error("sigemptyset failed\n");
    if (sigaddset(&signals, sig) < 0)
        unix_error("sigaddset failed\n");
    if (sigprocmask(how, &signals, NULL) < 0)
        unix_error("sigprocmask failed\n");
    return;
}

/* job函数开始   */
void clear_job(struct job_t *job)
{
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

void init_jobs(struct job_t *jobs)
{
    int i;
    for (i=0; i<MAXJOBS; i++)
        clear_job(&jobs[i]);
}

/* 返回最大分配的作业ID */
int max_jid(struct job_t *jobs)
{
    int i,max=0;

    for (i=0; i<MAXJOBS; i++)
        if (jobs[i].jid > max)
            max = jobs[i].jid;
    return max;
}

/* 增加作业到作业列表  */
int add_job(struct job_t *jobs, pid_t pid, int state, char *cmdline)
{
    int i;

    if (pid < 1)
        return 0;

    for (i=0; i<MAXJOBS; i++){
        if (jobs[i].pid == 0){
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;   //如果作业ID超过了作业列表的大小,则重新置为1
            strcpy(jobs[i].cmdline, cmdline);
            if (verbose){   //如果命令行参数有-v时则执行此语句块
                printf("Added job [%d] %d %s\n",jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
        }
    }
    printf("Tried to create too many jobs\n");  //当作业列表被占满时
    return 0;
}

/* 删除作业 */
int delete_job(struct job_t *jobs, pid_t pid)
{
    //printf("delete job\n");
    int i;

    if (pid < 1)
        return 0;

    for (i=0; i<MAXJOBS; i++){
        if (jobs[i].pid == pid){
            clear_job(&jobs[i]);
            nextjid = max_jid(jobs)+1;  //很有技巧
            return 1;
        }
    }
    return 0;
}

/* 返回当前前台进程的PID,如果没有则为0 */
pid_t fgpid(struct job_t *jobs)
{
    int i;

    for (i=0; i<MAXJOBS; i++)
        if (jobs[i].state == FG)
            return jobs[i].pid;
    return 0;
}

/* 根据PID返回所查询的作业 */
struct job_t *get_job_pid(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return NULL;
    for (i=0; i<MAXJOBS; i++)
        if (jobs[i].pid == pid)
            return &jobs[i];
    return NULL;
}

/* 根据JID返回所查询的作业 */
struct job_t *get_job_jid(struct job_t *jobs, int jid)
{
    int i;

    if (jid < 1)
        return NULL;
    for (i=0; i<MAXJOBS; i++)
        if (jobs[i].jid == jid)
            return &jobs[i];
    return NULL;
}

/* 把进程ID映射到作业ID */
int pid2jid(pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;
    for (i=0; i<MAXJOBS; i++)
        if (jobs[i].pid == pid)
            return jobs[i].jid;
    return 0;
}

void list_jobs(struct job_t *jobs)
{
    int i;

    for (i=0; i<MAXJOBS; i++){
        if (jobs[i].pid != 0){
            printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
            switch (jobs[i].state) {
                case BG:
                    printf("Running ");
                    break;
                case FG:
                    printf("Foreground ");
                    break;
                case ST:
                    printf("Stopped ");
                    break;
                default:
                    printf("listjobs: Internal error: job[%d].state=%d ",
                            i, jobs[i].state);
            }
            printf("%s\n", jobs[i].cmdline);
        }
    }
    //exit(0);  会直接退出整个程序
}

/* 打印帮助信息 */
void usage(void)
{
    printf("Usage:  shell [-hvp]\n");
    printf("   -h:  print this message\n");
    printf("   -v:  print additional diagnostic information\n");  //diagnostic是诊断的意思
    printf("   -p:  do not emit a cmmand prompt\n");
    exit(1);
}

void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/* sigaction的包装函数  */
handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

void Kill(pid_t pid, int signum)
{
    int rc;
    if ((rc = kill(pid, signum)) < 0)
        unix_error("kill error");
}
