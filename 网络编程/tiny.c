#include "csapp.h"
/*
 *说明:在用telnet时,Host头域指定请求资源的Intenet主机和端口号
 * 必须表示请求url的原始服务器或网关的位置。
 * HTTP/1.1请求必须包含主机头域，否则系统会以400状态码返回。
 */

void handler(int sig);
void doit(int fd);
void read_requesthdrs(rio_t *rp, int *length, int is_post_method);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, int is_head_method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head_method);
void post_dynamic(int fd, char *filename, int contentLength,rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv, char **environ)
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    /* 检查命令行参数 */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);

    listenfd = Open_listenfd(port);
    printf("Tiny Web Server...\n");
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
	
	/* fork子进程处理请求 */
	if (Fork() == 0) {
	    Close(listenfd);  /* 关闭监听描述符 */
	    doit(connfd);     
            Close(connfd);    /* 关闭已连接描述符 */
	    exit(0);
	}
	Close(connfd);   /* 父进程关闭已连接描述符,此时文件表项的引用计数为0,到客户端的连接终止 */
        
    }
    exit(0);
}


void handler(int sig)
{
    pid_t pid;

    if ((pid = waitpid(-1, NULL, 0)) < 0)
        unix_error("waitpid error");
    //printf("Handler reaped child %d\n", (int)pid);
    return;
}


void doit(int fd)
{
    int is_static, head = 0, post = 0, contentLength = 0;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd); /* 初始化缓冲区 */
    Rio_readlineb(&rio, buf, MAXLINE); /* 将缓冲区内的请求行(一行)内容读到buf里 */

    /* 读请求行 */
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("%s %s %s\r\n", method, uri, version);                  //11.6题扩展A

    if (!strcasecmp(method, "HEAD")) {
        head = 1;
    }
    else if (!strcasecmp(method, "POST")) {   /* 忽略大小写比较字符串 */
        post = 1;

    }
    else if (!strcasecmp(method, "GET"));
    else {
        clienterror(fd, method, "501", "Not Implemented", "Tiny does not implement this method");
        return;
    }

    read_requesthdrs(&rio, &contentLength, post);   /* 读取并忽略请求报头 */

    /* 解析来自GET请求的URI */
    is_static = parse_uri(uri, filename, cgiargs);   /* is_static判断请求的文件是否为静态文件 */
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't read the file");
        return;
    }

    if (is_static) {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
            return;
        }
        //printf("yes\n");
        serve_static(fd, filename, sbuf.st_size, head);    /* 处理静态内容 */
    }
    else {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
            return;
        }
        if (!post)
            serve_dynamic(fd, filename, cgiargs, head);       /* 处理动态内容 */
        else
            post_dynamic(fd, filename, contentLength, &rio);
    }
}


void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXLINE];

    /* 生成HTTP响应主体 */
    sprintf(body, "<html><head><meta charset='UTF-8'><title>Tiny Error</title></head>");
    sprintf(body, "%s<body bgcolor='#ffffff'>\r\n", body);
    sprintf(body, "%s<h2>%s: %s</h2>\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s</p>\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em></body></html>\r\n", body);  /* <hr>标签是水平分割线 */

    /* 打印HTTP响应 */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}


void read_requesthdrs(rio_t *rp, int *length, int is_post_method) {
    char buf[MAXLINE];
    char *p;

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {    /* 如果读到\r\n则结束循环,因为请求头以单独的\r\n一行结束 */
        Rio_readlineb(rp, buf, MAXLINE);
        if (is_post_method){
            if(strncasecmp(buf,"Content-Length:",15)==0) {
                p=&buf[15];
                p+=strspn(p," \t");
                *length=atol(p);
            }
        }
        printf("%s", buf);
        //printf("%d", *length);
    }
    //Rio_readlineb(rp, buf, MAXLINE);
    //Rio_readlineb(rp, buf, MAXLINE);

    return;
}


int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    /* 如果不是请求动态文件  */
    if (!strstr(uri, "cgi-bin")) {
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);   /* 将filename和uri连接起来,结果保存在filename里,即将uri转化为相对路径 */
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "index.html");  /* 如果uri以"/"结尾,把默认的文件名加载后面 */
        return 1;
    }
    else {
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1); /* 抽取出参数 */
            *ptr = '\0';
        }
        else {
            strcpy(cgiargs, "");
        }
        strcpy(filename, ".");
        strcat(filename, uri); /* 将uri转化为相对路径 */
        return 0;
    }
}


void serve_static(int fd, char *filename, int filesize, int is_head_method)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXLINE];

    /* 发送响应报头给客户端 */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n",  buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    //int fd2;                                                     11.6题扩展B
    //fd2 = Open("test.txt", O_WRONLY, 0);                         11.6题扩展B
    Rio_writen(fd, buf, strlen(buf));
    //Rio_writen(fd2, buf, strlen(buf));                           11.6题扩展B
    /* 发送响应主体给客户端 */
    if (is_head_method)
        return;

    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);  /* 分配了虚拟地址空间就不需要read/write了,也就不需要文件描述符 */

    //srcp = (char *)Malloc(filesize * sizeof(char));              11.9题
    //Rio_readn(srcfd, srcp, filesize);                            11.9题
    Rio_writen(fd, srcp, filesize);
    //free(srcp);                                                  11.9题

    //Rio_writen(fd2, srcp, filesize);                             11.6题扩展B
    Munmap(srcp, filesize);
}


void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg") || strstr(filename, ".jpeg"))
        strcpy(filetype,  "image/jpeg");
    else if (strstr(filename, ".mpg"))                        //11.6题扩展C
        strcpy(filetype, "video/mpg");
    else
        strcpy(filetype, "text/plain");
}


void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head_method)
{
    char buf[MAXLINE], *emptylist[] = {NULL};

    /* 打印一部分HTTP响应信息 */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n",  buf);
    Rio_writen(fd, buf, strlen(buf));

    if (is_head_method)
        return;

    Signal(SIGCHLD, handler);

    if (Fork() == 0) {
        setenv("QUERY_STRING", cgiargs, 1);  /* 只对当前进程有效 */
        Dup2(fd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);

    }
    //Wait(NULL);  /* 父进程监听子进程 */
}

/* 解决CGI读取POST参数的关键在于这个stdin该指向哪里呢,
 * 首先Dup2(fd, 0)是错的,CGI程序不可能从套接字流中读取,
 * 因为已经全部保存在rio缓冲区了,所以只能从缓冲区读取
 * 剩下的请求主体,而CGI程序如何和原来的进程联系从而获
 * 得请求主体呢?答案就是管道,利用dup2函数将标准输入流
 * 重定向到管道的读端,而缓冲区的数据则写到管道写端.花
 * 了我好长时间理解 
 */

void post_dynamic(int fd, char *filename, int contentLength,rio_t *rp)
{
    char buf[MAXLINE], data[MAXLINE], length[32], *emptylist[] = {NULL};
    int pipe_fd[2];

    sprintf(length,"%d",contentLength);
    memset(data,0,MAXLINE);

    Signal(SIGCHLD, handler);

     /* 创建管道 */
    if (pipe(pipe_fd) < 0) {
        perror("pipe failed\n");
        exit(errno);
    }

    if (Fork() == 0) {   /* 创建子进程 */
        Close(pipe_fd[0]);  /* 关闭管道读端 */
        Rio_readnb(rp, data, contentLength);  /* 从缓冲区中读取数据,此时缓冲区的内容中请求头已经
                                               * 被read_requesthdrs函数读掉了,剩下的是请求主体内容
                                               * 但是不要使用Rio_readlineb,因为这个函数一次只能一行
                                               * 用Rio_readnb读取全部请求主体
                                               */
        Rio_writen(pipe_fd[1], data, contentLength); /* 将数据写进管道 */
        exit(0); /* 第一个子进程完成任务,退出 */
    }
    else {
        /* 打印一部分HTTP响应信息 */
        sprintf(buf, "HTTP/1.0 200 OK\r\n");
        sprintf(buf, "%sServer: Tiny Web Server\r\n",  buf);
        Rio_writen(fd, buf, strlen(buf));

        if (Fork() == 0) {
            Close(pipe_fd[1]);   /* 子进程关闭管道写端 */
            Dup2(pipe_fd[0], STDIN_FILENO); /* 让子进程不再从标准输入读取,而是从管道读取 */
            Close(pipe_fd[0]);  /* 关闭管道读端 */
            setenv("CONTENT_LENGTH", length, 1);
            Dup2(fd, STDOUT_FILENO); /* 重定向标准输出到客户端,让输出指向已连接的描述符 */
            Execve(filename, emptylist, environ);
        }
        else {
            /* 父进程关闭读端和写端 */
            close(pipe_fd[0]);
            close(pipe_fd[1]);
        }
    }

}
