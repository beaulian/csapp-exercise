#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd, port;
    char *host, buf[MAXLINE];
    rio_t rio;    /* 缓冲区 */

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
     /* 获得将要访问的ip地址 */
    host = argv[1];
    port = atoi(argv[2]);

    clientfd = Open_clientfd(host, port);  /* 返回建立好连接的套接字描述符 */
    Rio_readinitb(&rio, clientfd); /* 创建一个读缓冲区,并将读缓冲区和clientfd文件描述符联系起来 */

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }

    Close(clientfd);
    exit(0);
}