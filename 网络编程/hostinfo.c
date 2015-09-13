#include "csapp.h"

int main(int argc, char **argv)
{
    char **pp;
    struct in_addr addr;
    struct hostent *hostp;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <domain name or dotted-decimal>\n", argv[0]);
        exit(0);
    }

    if (inet_aton(argv[1], &addr) != 0)
        hostp = Gethostbyaddr((const char *)&addr, sizeof(addr), AF_INET);  //AF_INET等于0,指的是使用IPV4
    else
        hostp = Gethostbyname(argv[1]);

    printf("official hostname: %s\n", hostp->h_name);

    for (pp = hostp->h_aliases; *pp != NULL; pp++) {  //aliase->别名
        printf("alias: %s\n", *pp);
    }

    for (pp = hostp->h_addr_list; *pp != NULL; pp++) {
        //printf("%s\n", *pp);
        addr.s_addr = ((struct in_addr *)*pp)->s_addr;   //*pp 是 char *类型
        /* expected ‘struct in_addr’ but argument is of type ‘struct in_addr *’ */
        printf("address: %s\n", inet_ntoa(addr));
    }
    exit(0);
}