#include "csapp.h"

/* 水平不够,POST功能未完成,还请看到的大神能指点一下 */

/*
 * 说明:在用telnet时,Host头域指定请求资源的Intenet主机和端口号
 * 必须表示请求url的原始服务器或网关的位置。
 * HTTP/1.1请求必须包含主机头域，否则系统会以400状态码返回。
 */

void handler(int sig);
void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, int is_head_method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head_method, int is_post_method);
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
        doit(connfd);
        Close(connfd);
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
    int is_static, head = 0, post = 0;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd); /* 初始化缓冲区 */
    Rio_readlineb(&rio, buf, MAXLINE); /* 将缓冲区内的内容读到buf里 */

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

    read_requesthdrs(&rio);   /* 读取并忽略请求报头 */

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
        serve_dynamic(fd, filename, cgiargs, head, post);       /* 处理动态内容 */
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


void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {    /* 如果读到\r\n则结束循环 */
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
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


void serve_dynamic(int fd, char *filename, char *cgiargs, int is_head_method, int is_post_method)
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
        if (!is_post_method) {
            setenv("QUERY_STRING", cgiargs, 1);  /* 只对当前进程有效 */
            Dup2(fd, STDOUT_FILENO);
            Execve(filename, emptylist, environ);
        }
        else {
            //printf("yes\n");

            Dup2(fd, STDOUT_FILENO);
            Dup2(fd, STDIN_FILENO);
            Execve(filename, emptylist, environ);
        }

    }
    //Wait(NULL);  /* 父进程监听子进程 */
}


