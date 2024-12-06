#include "stdlib.h"
#include "types.h"
int x = 1;
int first_task_main(void)
{

    int count = 3;

    int pid = getpid();
    printf_tmp("first task id=%d \r\n", pid);
    int i;
    for (i = 0; i < 5; i++)
    {
        pid = fork();
        if (pid < 0)
        {
            //printf_tmp("create child proc failed.\r\n", 0);
        }
        else if (pid == 0)
        {
            //printf_tmp("child: %d\r\n", getpid());
            x = 2;
            //printf_tmp("x = %d\r\n",x);
            char *argv[] = {"arg0", "arg1", "arg2", "arg3", NULL};
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

    

    return 0;
}