#include "csapp.h"

void echo(int connfd);
void command(void);

int main(int argc, char **argv)
{
    int listenfd, connfd, port;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;   /* 套接字地址结构 */
    struct hostent *hp;              /* DNS主机条目结构 */
    char *haddrp;
    fd_set read_set, ready_set;   //建立两个描述符集合

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);

    FD_ZERO(&read_set);  //清空读集合
    FD_SET(STDIN_FILENO, &read_set);  //增加标准输入到读集合
    FD_SET(listenfd, &read_set);      //增加listened到读集合

     while (1) {
        ready_set = read_set;
        Select(listenfd+1, &ready_set, NULL, NULL, NULL);
        if (FD_ISSET(STDIN_FILENO, &ready_set))   //看标准输入是否可读写
            command();
        if (FD_ISSET(listenfd, &ready_set)) {

            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

            hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                            sizeof(clientaddr.sin_addr.s_addr), AF_INET);
            haddrp = inet_ntoa(clientaddr.sin_addr);
            printf("server connetced to %s (%s)\n", hp->h_name, haddrp);
            echo(connfd);
            Close(connfd);
        }
     }
     exit(0);
}

void echo(int connfd)
{
    size_t n;    /* size_t在64微操作系统中代表 long unsigned int */;

    char buf[MAXLINE];
    rio_t rio
    Rio_readinitb(&rio, connfd);   /* rio缓冲区读connfd中的数据 */
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %lu bytes\n", n);
        Rio_writen(connfd, buf, n);
    }
}

void command(void)
{
    char buf[MAXLINE];
    if (!Fgets(buf, MAXLINE, stdin))
        exit(0);   //EOF
    printf("%s", buf);    //处理输入的命令
}

