#include "main.h"
int x = 0;
int main(int argc, char *argv[])
{

    // int ret = fork();
    // if (ret == 0)
    // {
    //     while (1)
    //     {
    //         printf_tmp("child task id %d\r\n", getpid());
    //         x = 5;
    //         sleep(1000);
    //     }
    // }
    // else if (ret > 0)
    // {
    //     while (1)
    //     {
    //         x = 2;
    //         int pid = getpid();
    //         printf_tmp("father task id %d ,", pid);
    //         printf_tmp("create a child id is %d\r\n", ret);
    //         sleep(1000);
    //     }
    // }
    // else
    // {
    //     printf_tmp("fork faile\r\n", 1);
    // }

    int count = 0;
    printf_tmp("Number of arguments: %d\r\n", argc);
    for (int i = 0; i < argc; i++) {
        printf_tmp("Argument  %s\r\n",  argv[i]);
    }
    while (1)
    {
        count++;
        printf_tmp("count is %d\r\n", count);
        sleep(1000);
    }
}