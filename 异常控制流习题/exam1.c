#include "csapp.h"

int main()
{
    if (fork() == 0){
        printf("a");
        exit(0);
    }
    else{
        printf("b");
        waitpid(-1,NULL,0);
    }
    printf("c");
    exit(0);
}