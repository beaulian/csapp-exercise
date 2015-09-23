#include "csapp.h"

void echo(int connfd);
void *thread(void *vargp);

int main(int argc, char **argv)
{
    int listenfd, *connfdp, port;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);
    while (1) {
        connfdp = Malloc(sizeof(int)); /* 分配一个大小为4字节的整型指针 */
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Pthread_create(&tid, NULL, thread, connfdp);
    }
}

void *thread(void *vargp)
{
    int connfd = *(int *)vargp;
    Pthread_detach(pthread_self());  /* 分离线程 */
    Free(vargp);
    echo(connfd);
    Close(connfd);
    return NULL;
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