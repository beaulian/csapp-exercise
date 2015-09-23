#include "csapp.h"

void echo(int connfd);
void sigchld_handler(int sig);

int main(int argc, char **argv)
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;   /* 套接字地址结构 */
    struct hostent *hp;              /* DNS主机条目结构 */
    char *haddrp;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);
    Signal(SIGCHLD, sigchld_handler);
    listenfd = Open_listenfd(port);
     while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        if (Fork() == 0) {
            Close(listenfd);

            hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                            sizeof(clientaddr.sin_addr.s_addr), AF_INET);
            haddrp = inet_ntoa(clientaddr.sin_addr);
            printf("server connetced to %s (%s)\n", hp->h_name, haddrp);

            echo(connfd);
            Close(connfd);
            exit(0);  //子进程退出
        }
        Close(connfd);    //父进程关闭已连接套接字描述符

     }
     exit(0);
}

void echo(int connfd)
{
    size_t n;    /* size_t在64微操作系统中代表 long unsigned int */
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);   /* rio缓冲区读connfd中的数据 */
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %lu bytes\n", n);
        Rio_writen(connfd, buf, n);
    }
}

void sigchld_handler(int sig)
{
    while (waitpid(-1, 0, WNOHANG) > 0)   //WNOHANG是若pid指定的子进程没有结束，则waitpid()函数返回0，不予以等待
        ;
    return;
}