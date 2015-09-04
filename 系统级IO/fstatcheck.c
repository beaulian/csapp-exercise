#include "csapp.h"

int main(int argc, char **argv)
{
    struct stat stat;
    char *type, *readok;

    //printf("%d\n", *argv[1]-'0');
    //return 0;

    /* 很重要 */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <fd>\n", argv[0]);
        exit(0);
    }

    Fstat(*argv[1]-'0', &stat);
    if (S_ISREG(stat.st_mode))
        type = "regular";
    else if (S_ISDIR(stat.st_mode))
        type = "directory";
    else
        type = "other";

    /* 检查读权限 */
    if ((stat.st_mode & S_IWUSR))
        readok = "yes";
    else
        readok = "no";

    printf("type: %s, read: %s\n", type, readok);
    exit(0);
}