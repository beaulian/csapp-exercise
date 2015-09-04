#include "csapp.h"

void mmapcopy(int fd, int size)
{
    char *bufp;
    bufp = Mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    Write(1, bufp, size);  //文件描述符1代表标准输出流
    return;
}

int main(int argc, char **argv)
{
    struct stat stat;
    int fd;

    if (argc != 2) {
        printf("usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    fd = Open(argv[1], O_RDONLY, 0);
    fstat(fd, &stat);
    mmapcopy(fd, stat.st_size);
    munmap(NULL, stat.st_size);   //删除创建的虚拟存储器区域
    exit(0);
}