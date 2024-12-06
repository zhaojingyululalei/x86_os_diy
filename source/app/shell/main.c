#include "main.h"
int x = 0;
int main(int argc, char *argv[])
{
    printf_tmp("shell create some child\r\n",1);
  

    int pid = getpid();
    printf_tmp("shell task id=%d \r\n", pid);
    int i;
    for (i = 0; i < 2; i++)
    {
        pid = fork();
        if (pid < 0)
        {
            printf_tmp("create child proc failed.\r\n", 0);
        }
        else if (pid == 0)
        {
            printf_tmp("child: %d\r\n", getpid());
            //x = 2;
            printf_tmp("x = %d\r\n",x);
            
            sleep(1000);
            exit(0);
        }
        else
        {
            printf_tmp("child task id=%d  ", pid);
        }
    }
    return 0;
    for (int i = 0; i < 2; i++)
    {
        int status, pid;
        pid = wait(&status);
        printf_tmp("collect on child %d\r\n", pid);
    }
    

        
  
    
    return 0;
}