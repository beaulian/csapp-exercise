#include "csapp.h"
#define N 2
void *thread(void *vargp);

char **ptr; /* 全局变量 */

int main()
{
    int i;
    pthread_t tid;
    char *msgs[N] = {
        "Hello from foo",
        "Hello from bar"
    };

    ptr = msgs;
    for (i=0; i<N; i++)
        Pthread_create(&tid, NULL, thread, (void *)(long)i);
    Pthread_exit(NULL);
}

void *thread(void *vargp)
{
    int myid = (long int)vargp;
    static int cnt = 0;
    printf("[%d]: %s (cnt=%d)\n", myid, ptr[myid], ++cnt);
    return NULL;
}