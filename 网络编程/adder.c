#include "csapp.h"

int main(void)
{
    char *buf, *query, *env, *p1, *p2, *p3;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE], postdata[MAXLINE], *lenstr;
    int n1=0, n2=0;
    long len=0;

    /* 提取两个参数  */
    query = getenv("QUERY_STRING");
    if (query == NULL) {
        if ((lenstr = getenv("CONTENT_LENGTH")) != NULL)
            len = atol(lenstr);
        fgets(postdata, len+1, stdin);
        buf = postdata;
    }
    else
        buf = query;

    p1 = strchr(buf, '&');
    *p1 = '\0';

    /* 提取第一个参数 */
    p2 = strchr(buf, '=');
    *p2 = '\0';
    strcpy(arg1, p2+1);

    /* 提取第二个参数 */
    p3 = strchr(p1+1, '=');
    *p3 = '\0';
    strcpy(arg2, p3+1);

    n1 = atoi(arg1);
    n2 = atoi(arg2);

     /* 生成响应主体 */
    sprintf(content, "%sTHE Internet addition portal,\r\n<p>", content);
    sprintf(content, "%sThe answer is: %d + %d = %d</p>\r\n<p>",
                        content, n1, n2, n1+n2);
    sprintf(content, "%sThanks for visiting!</p>\r\n", content);

    /* 生成HTTP响应 */
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);
    exit(0);

}
