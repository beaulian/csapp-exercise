#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

pthread_t tid[10];

void *thread_handler(void *arg)
{
    /* 因为64位系统的空类型指针的大小为8字节,所以要用long int,不然在强制类型转换的时候会报错 */
    printf("tid%ld:%u,pid:%u\n", (long int)arg, (unsigned)pthread_self(), (unsigned)getpid());
    while(1){
        sleep(1);
    }
    return NULL;
}

int main(void)
{
    int i, ret;
    pid_t pid;
    printf("main tid:%u,pid:%u\n", (unsigned)pthread_self(),
        (unsigned)getpid());
    for(i = 0; i < 10; i++){
        if((ret = pthread_create(&tid[i], NULL, thread_handler, (void *)(long)i)) != 0){
            fprintf(stderr, "pthread_create:%s\n", strerror(ret));
            exit(1);
        }
    }
    sleep(3);
    pid = fork();
    if(pid == 0){
        printf("son tid:%u,pid:%u\n", (unsigned)pthread_self(),
            (unsigned)getpid());
        while(1);
            sleep(1);
    }
    while(1)
        sleep(2);
    exit(0);
}