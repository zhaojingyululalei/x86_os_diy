#include "stdlib.h"
int x=1;
int first_task_main(void)
{
    int count = 0;

    int ret = fork();
    if (ret == 0)
    {
        while (1)
        {
            printf_tmp("child task id %d\r\n", getpid());
            x = 5;
            sleep(1000);
        }
    }
    else if (ret > 0)
    {
        while (1)
        {
            x = 2;
            int pid = getpid();
            printf_tmp("father task id %d ,",pid);
            printf_tmp("create a child id is %d\r\n", ret);
            sleep(1000);
        }
    }
    else
    {
        printf_tmp("fork faile\r\n", 1);
    }
    while (1)
    {
        sleep(1000);
    }

    return 0;
}