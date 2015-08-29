#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<errno.h>

int main(int argc, char *argv[])
{
    extern char **environ;
    int status;
    pid_t pid[2];   //存放fork返回值
    int pipe_fd[2]; /* 存放管道的读写端的描述符,
                     * pipe_fd[0]为管道里的读端,
                     * pipe_fd[1]为管道里的写端
                     */

    char *prog1_argv[4] = {"/bin/ls", "-l", "./", NULL};
    char *prog2_argv[2] = {"/bin/more", NULL};

    /* 创建管道 */
    if (pipe(pipe_fd) < 0) {
        perror("pipe failed\n");
        exit(errno);
    }

    /* 父进程为ls命令创建子进程 */
    if ((pid[0] = fork()) < 0) {
        perror("fork failed\n");
        exit(errno);
    }

    /* ls子进程 */
    if (pid[0] == 0) {

        /* 程序完成ls命令,无须外界输入,直接关闭读端 */
        close(pipe_fd[0]);

        /* 将管道的写描述符复制给标准输出,然后关闭 */
        dup2(pipe_fd[1],1); /* 让输出不再指向标准输出,
                             * 而是指向管道的写端
                             */
        close(pipe_fd[1]);
        execve(prog1_argv[0], prog1_argv, environ);
    }
    else {
        if ((pid[1] = fork()) < 0) {
            perror("fork failed\n");
            exit(errno);
        }

        if (pid[1] == 0) {
            close(pipe_fd[1]);   //直接关闭写端

            /* 将管道的读描述符复制给标准输入,然后关闭 */
            dup2(pipe_fd[0], 0);
            close(pipe_fd[0]);
            /* 执行more命令  */
            execve(prog2_argv[0], prog2_argv, environ);
        }

        //父进程
        else {
            /* 父进程关闭读端和写端 */
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            waitpid(pid[1], &status, 0);
            printf("Done waiting for more\n");
        }
    }
}

