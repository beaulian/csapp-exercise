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
}