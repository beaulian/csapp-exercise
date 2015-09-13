#include "csapp.h"

int main(int argc, char **argv)
{
    unsigned int addr;

    struct in_addr inaddr;

    if (argc != 2) {
        unix_error("please input a argument");
        exit(-1);
    }

    sscanf(argv[1], "%x", &addr);
    inaddr.s_addr = htonl(addr);   //将主机字节顺序转为网络字节顺序(从小端法转为大端法)
    printf("%s\n",inet_ntoa(inaddr));

    exit(0);
}