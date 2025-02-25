#include "_stdlib.h"
#include "types.h"
// 字符串长度函数 strlen
int x_strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}
int x = 1;
int first_task_main(void)
{



    int count = 3;

    int pid = getpid();
    printf_tmp("first task id=%d \r\n", pid);
    int i;
    for (i = 0; i < 8; i++)
    {
        pid = fork();
        if (pid < 0)
        {
            //printf_tmp("create child proc failed.\r\n", 0);
        }
        else if (pid == 0)
        {
            //printf_tmp("child: %d\r\n", getpid());
            
            //printf_tmp("x = %d\r\n",x);
            char* ttyn = "ttyn";
            int end = x_strlen(ttyn)-1;
            ttyn[end] = '0'+i;
            char *argv[] = {ttyn, NULL};
            execve("shell.elf", argv, (char **)0);
            
        }
        else
        {
            //printf_tmp("child task id=%d  ", pid);
            //printf_tmp("parent: %d\r\n", count);
        }
    }

    while (1)
    {

        int status, pid;
        pid = wait(&status);
        printf_tmp("collect on child %d\r\n", pid);
    }

    // char* str = malloc(1024);
    // for (int i = 0; i < 1022; i++)
    // {
    //     str[i] = 'a';
    // }
    
    // printf_tmp("%c\r\n",str[1021]);
    // free(str);
    // while (1)
    // {
    //     sleep(1000);
    // }
    

    return 0;
}