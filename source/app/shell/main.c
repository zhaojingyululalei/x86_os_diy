#include "main.h"
#include "_stdio.h"
#include "string.h"



#define CMD_BUF_MAX_LEN 1024

// 显示 help 信息
void help_command() {
    printf("Available commands:\r\n");
    printf("help - Show available commands\r\n");
    printf("echo <message> - Print a message\r\n");
    printf("exit - Exit the shell\r\n");
    printf("clear - clear the screen\r\n");
    
    
}

// 处理 echo 命令
void echo_command(char *args[], int argc) {
    // 输出 echo 后的所有参数
    for (int i = 1; i < argc; i++) {
        fputs(1, args[i], strlen(args[i]));
        if (i != argc - 1) {
            fputs(1, " ", 1); // 参数之间加空格
        }
    }
    fputs(1, "\n", 1); // 输出换行符
}

// 处理 exit 命令
void exit_command() {
    exit(0); // 调用提供的 exit 函数退出
}

void clear_command(){
    printf("%s", ESC_CLEAR_SCREEN);
    printf("%s", ESC_MOVE_CURSOR(0, 0));
    return 0;
}

// 命令解析并执行
void execute_command(char *cmd) {
    char *args[CMD_BUF_MAX_LEN / 2];  // 用于存储命令的参数
    int argc = 0;
    char *token = strtok(cmd, " \n");

    // 将命令按空格分割成参数
    while (token != NULL) {
        args[argc++] = token;
        token = strtok(NULL, " \n");
    }

    if (argc == 0) return;

    // 处理不同的命令
    if (strncmp(args[0], "help",strlen("help")) == 0) {
        help_command();  // 调用 help 命令
    }
    else if (strncmp(args[0], "echo",strlen("echo")) == 0) {
        echo_command(args, argc);  // 调用 echo 命令
    }
    else if (strncmp(args[0], "exit",strlen("exit")) == 0) {
        exit_command();  // 调用 exit 命令
    }else if(strncmp(args[0], "clear",strlen("clear"))==0){
        clear_command();
    }
    else {
        printf("unkonw commond:");
        fputs(1, args[0], strlen(args[0]));
        fputs(1, "\n", 1);
    }
}

int main(int argc, char *argv[]) {
    char cmd_buf[CMD_BUF_MAX_LEN] = {0};

    // 模拟 shell 的标准输入输出
    int fd = open(argv[0], 0, 0); // stdin
    dup(fd);  // stdout
    dup(fd);  // stderr

    while (1) {
        // 打印提示符
        fputs(1, "$ ", 2);

        // 获取用户输入
        if (fgets(0, cmd_buf, CMD_BUF_MAX_LEN) == 0) {
            break;  // 如果没有输入则退出
        }

        // 执行命令
        execute_command(cmd_buf);
    }

    return 0;
}
