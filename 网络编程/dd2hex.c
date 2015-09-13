#include "csapp.h"

int main(int argc, char **argv)
{
    struct in_addr inp;
    //struct in_addr *inp;   //如果直接使用结构体指针,这样是个野指针,如果不分配内存,会出现段错误
    //inp = (struct in_addr *)malloc(sizeof(struct in_addr));
    int n;

    if (argc != 2) {
        unix_error("please input a argument\n");
        exit(-1);
    }

    if ((n = inet_aton(argv[1], &inp)) != 1) {
        unix_error("can't change\n");
        exit(-1);
    }

    printf("0x%x\n", ntohl(inp.s_addr));
    exit(0);
}