#include "csapp.h"

void echo(int connfd);

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

    listenfd = Open_listenfd(port);
     while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                            sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        haddrp = inet_ntoa(clientaddr.sin_addr);
        printf("server connetced to %s (%s)\n", hp->h_name, haddrp);

        echo(connfd);
        Close(connfd);
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