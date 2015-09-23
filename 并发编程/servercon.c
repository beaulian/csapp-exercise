#include "csapp.h"

/* 基于I/O多路复用的并发事件驱动服务器 */

/* 表示一个已连接描述符池 */
typedef struct {
    int maxfd;                    /* read_set集合中最大的描述符 */
    fd_set read_set;              /* 所有读描述符的集合 */
    fd_set ready_set;             /* 准备好读写的描述符的集合,是read_set的子集 */
    int nready;                   /* 从select返回的可读描述符数量 */
    int maxi;                     /* client数组的最高索引 */
    int clientfd[FD_SETSIZE];     /* 描述符的集合 */
    rio_t clientrio[FD_SETSIZE];  /* 读缓冲区 */
} pool;

void init_pool(int listenfd, pool *p);  /* 初始化池 */
void add_client(int connfd, pool *p);
void check_client(pool *p);

int byte_cnt = 0;   /* 服务器接收的总字节数 */

int main(int argc, char **argv)
{
    int listenfd, connfd, port;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    static pool pool;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);
    printf("server...\n");
    listenfd = Open_listenfd(port);
    init_pool(listenfd, &pool);  /* 初始化池 */

    while (1) {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

        /* 如果监听米描述符已准备好,则增加新的客户端到池中 */
        if (FD_ISSET(listenfd, &pool.ready_set)) {
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            add_client(connfd, &pool);
        }

        /* 回送每个准备好的已连接描述符的一个文本行 */
        check_client(&pool);
    }
    return 0;
}

void init_pool(int listenfd, pool *p)
{
    int i;
    p->maxi = -1;
    for (i=0; i<FD_SETSIZE; i++)
        p->clientfd[i] = -1;

    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p)
{
    int i;
    p->nready--;
    for (i=0; i<FD_SETSIZE; i++)
        if (p->clientfd[i] < 0) {
            /* 增加已连接描述符到池中 */
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);  /* 初始化缓冲区 */
            FD_SET(connfd, &p->read_set);

            /* 更新最大的描述符和最大的索引 */
            if (connfd > p->maxfd)
                p->maxfd = connfd;
            if (i > p->maxi)
                p->maxi = i;
            break;
        }
    if (i == FD_SETSIZE)
        app_error("add_client error: Too many clients");
}

void check_client(pool *p)
{
    int i, connfd, n;
    char buf[MAXLINE];
    rio_t rio;
    /* nready是已准备好的描述符数 */
    for (i=0; (i <= p->maxi) && (p->nready > 0); i++) {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];

        /* 如果描述符已经准备好了,打印一行回送给客户端 */
        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
                byte_cnt += n;
                printf("Server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
                Rio_writen(connfd, buf, n);
            }
            /* EOF detected, remove descriptor from pool */
            else {
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }
        }
    }
}