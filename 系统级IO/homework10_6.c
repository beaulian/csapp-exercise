#include "csapp.h"

int main()
{
    int fd1, fd2;

    fd1 = Open("buf.c", O_RDONLY, 0);
    fd2 = Open("csapp.c", O_RDONLY, 0);
    Close(fd2);
    fd2 = Open("cpfile.c", O_RDONLY, 0);
    printf("fd2 = %d\n", fd2);
    exit(0);
}