#include "csapp.h"

void *thread(void *vargp);

int main()
{
    /* pthread_t的类型为unsigned long int,
     * 所以在打印的时候要使用%lu方式，否则显示结果出问题
     */
    pthread_t tid;
    Pthread_create(&tid, NULL, thread, NULL);
    Pthread_join(tid, NULL);
    exit(0);
}

void *thread(void *vargp)
{
    printf("Hello, world!\n");
    printf("tid=%lu\n", pthread_self());
    return NULL;
}