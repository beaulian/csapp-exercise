#include "csapp.h"

int main(int argc, char **argv)
{
    int n, fd;
    rio_t rio;
    char buf[MAXBUF];

    if ((argc != 1) && (argc != 2)) {
        fprintf(stderr, "usage: %s <infile>\n", argv[0]);
        exit(1);
    }

    if (argc == 2) {
        if ((fd = Open(argv[1], O_RDONLY, 0)) < 0) {
            fprintf(stderr, "Couldn't read %s\n", argv[1]);
            exit(1);
        }
        Dup2(fd, STDIN_FILENO);
        Close(fd);   /* 避免浪费存储器资源 */
    }

    Rio_readinitb(&rio, STDIN_FILENO);
    while ((n = Rio_readlineb(&rio, buf, MAXBUF)) != 0)
        Rio_writen(STDOUT_FILENO, buf, n);
}