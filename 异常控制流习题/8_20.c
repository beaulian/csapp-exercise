#include "csapp.h"

int main(int argc,char *argv[],char *envp[])
{
    char *myls = "myls"; //取任意字符串都可以
    if (!strcmp(myls,argv[1])){
        execve("/bin/ls",argv+1,envp);  //忽略第一个参数./myls
    }

}

