#include "stdlib.h"
int first_task_main(void)
{
    int count = 0;
    while (1)
    {
        int pid = getpid();
        printf_tmp("task pid is %d\r\n",pid);
        sleep(1000);
        
    }
    return 0;
}