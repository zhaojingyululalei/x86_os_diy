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
            printf_tmp("create child proc failed.\r\n", 0);
        }
        else if (pid == 0)
        {
            printf_tmp("child: %d\r\n", getpid());

            char *argv[] = {"arg0", "arg1", "arg2", "arg3", NULL};
            execve("shell.elf", argv, (char **)0);
            while (1)
            {
                ;
            }
        }
        else
        {
            printf_tmp("child task id=%d  ", pid);
            printf_tmp("parent: %d\r\n", count);
        }
    }

    while (1)
    {

        int status, pid;
        pid = wait(&status);
        printf_tmp("collect on child %d\r\n", pid);
    }

    // for (int i = 0; i < 1; i++) {
    //     int pid = fork();
    //     if (pid < 0) {
    //         printf_tmp("create shell proc failed", 0);
    //         break;
    //     } else if (pid == 0) {
    //         // 子进程
    //         char* a = "hello";
    //         char* b = "world";
    //         char* c = "zhao";
    //         char* d = "yu";
    //         char * argv[] = {a,b, NULL};
    //         char* env[] = {c,d,NULL};
    //         execve("shell.elf", argv, env);
    //         printf_tmp("create shell proc failed", 0);
    //         while (1) {
    //             sleep(1000);
    //         }
    //     }
    // }

    // while (1)
    // {
    //     printf_tmp("task id is %d\r\n",getpid());
    //     sleep(1000);
    // }

    return 0;
}