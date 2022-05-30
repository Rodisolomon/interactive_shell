#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
int num_arg; //num of argument
int num_cmd;
int MAX_BYTE = 5;
void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}
void rais_err() {
    char error_message[30] = "An error has occurred\n";
    write(STDOUT_FILENO, error_message, strlen(error_message));
}

int too_long(char* cmd_buff) { //check if the line is too long
    int too_long = 1;
    if ((cmd_buff[MAX_BYTE - 1] == '\0') || (cmd_buff[MAX_BYTE - 1] == '\n'))
        too_long = 0; 
    return too_long;
}

int handle_too_long_cmd(char* cmd_buff) { //return 1 if the command line is too long, handle too-long-cmd
    int too_long_command = 0;
    if (too_long(cmd_buff)) {
        too_long_command = 1;
        rais_err();
        myPrint(cmd_buff);
        while (too_long(cmd_buff)) {
            memset(cmd_buff, '\0', (MAX_BYTE + 1));
            fgets(cmd_buff, (MAX_BYTE + 1), stdin);
            myPrint(cmd_buff);
        }
    }
    return too_long_command;
}

int empty_space(char* token) { //return 1 if the string is meaningless
    while (*token != '\0') {
        if (!isspace((char)*token))
            return 1;
        token++;
    }
    return 0;
}

char** create_cmd_list(char* cmd_buff) {
    //separate command, create command list
    const char s[2] = ";";
    char* token; 
    num_cmd = 0;
    char* cmd_buffer[520];
    token = strtok(cmd_buff, s);
    while (token != NULL) {
        //deal with empty string
        if (empty_space(token))
            continue; //jump directly to next loop
        printf("token in cmd list separated by ; is %s\n", token);
        cmd_buffer[num_cmd] = token;
        num_cmd++;
        token = strtok(NULL, s);
    }
    char** cmd_list = malloc(sizeof(char*) * (num_cmd));
    for (int i = 0; i < num_cmd; i++) {
        cmd_list[i] = cmd_buffer[i];
    }
    return cmd_list;
}

char** create_arg_list(char* single_cmd) {
    //separate argument, create argument list for execvp
    num_arg = 0;
    char* arg_buffer[20]; //a buffer to hold all arguments

    const char s[2] = " ";
    char* token;
    token = strtok(single_cmd, s);
    while (token != NULL) {
        printf("token in a single commad is %s\n", token);
        arg_buffer[num_arg] = token;
        num_arg++;
        token = strtok(NULL, s);
    }

    char** arg_list = malloc(sizeof(char*) * (num_arg+1));
    for (int i = 0; i < num_arg; i++) {
        arg_list[i] = arg_buffer[i];
    }
    arg_list[num_arg] = NULL;
    return arg_list;
}


int main(int argc, char *argv[]) 
{
    char cmd_buff[MAX_BYTE + 1]; //initiate
    char *pinput;

    while (1) {
        myPrint("myshell> ");
        if (fgets(cmd_buff, (MAX_BYTE + 1), stdin) == NULL)
            break;


        printf("\ncmd_buff: %c, %c, %c, %c, %c, %c\n", cmd_buff[0], cmd_buff[1], cmd_buff[2], cmd_buff[3], cmd_buff[4], cmd_buff[5]);
        printf("\ncmd_buff: %d, %d, %d, %d, %d, %d\n", cmd_buff[0], cmd_buff[1], cmd_buff[2], cmd_buff[3], cmd_buff[4], cmd_buff[5]);
    
        if (handle_too_long_cmd(cmd_buff))
            continue;
        
        char** cmd_list = create_cmd_list(cmd_buff);
        for (int i = 0; i < num_cmd; i++) {
            printf("cmd we got is %s\n", cmd_list[i]);
            char** arg_list = create_arg_list(cmd_list[i]);
            printf("first and second arg we got is %s %s\n", arg_list[0], arg_list[1]);
        }
        memset(cmd_buff, '\0', (MAX_BYTE + 1));
    }
}

