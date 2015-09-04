/* 查看printf缓冲区大小 */

#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int i = 0x61;
    while(1)
    {
        printf("%c", i);
        i++;
        i = (i - 0x61) % 26 + 0x61;
        usleep(1000);

    }
    return 0;
}