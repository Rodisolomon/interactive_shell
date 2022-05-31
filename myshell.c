#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>

int num_arg; //num of argument
int num_cmd;
int MAX_BYTE = 513;
char* exit_str = "exit\0";
char* cd_str = "cd\0";
char* pwd_str = "pwd\0";
int last_empty = 0; //prevent multple myshell>>



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
    //printf("cmd_buff[MAX_BYTE - 1] is %d\n", cmd_buff[MAX_BYTE - 1]);
    if ((cmd_buff[MAX_BYTE - 1] == '\0') || (cmd_buff[MAX_BYTE - 1] == '\n'))
        too_long = 0; 
    return too_long;
}

int handle_too_long_cmd(char* cmd_buff, FILE * fp) { //return 1 if the command line is too long, handle too-long-cmd
    int too_long_command = 0;
    if (too_long(cmd_buff)) {
        too_long_command = 1;
        myPrint(cmd_buff);
        while (too_long(cmd_buff)) {
            memset(cmd_buff, '\0', (MAX_BYTE + 1));
            fgets(cmd_buff, (MAX_BYTE + 1), fp);
            myPrint(cmd_buff);
        }
    }
    return too_long_command;
}

int empty_space(char* token) { //return 1 if the string is meaningless
    while (*token != '\0') {
        if ( !( isspace((char)*token) ) ) {
            return 0;
        }
        token++;
    }
    return 1;
}

char** create_cmd_list(char* cmd_buff) {
    int all_empty = 1;
    char original_cmds[514];
    strcpy(original_cmds, cmd_buff); //方便print if not empty
    //printf("cmd_list before is : %s\n", original_cmds);
    //separate command, create command list
    const char s[2] = ";";
    char* token; 
    num_cmd = 0;
    char* cmd_buffer[520];
    token = strtok(cmd_buff, s);
    while (token != NULL) {
        //deal with empty string
        if (empty_space(token)) {
            token = strtok(NULL, s);
            //printf("%s\n", token);
            continue; //jump directly to next loop
        }
        all_empty = 0;
        //printf("token in cmd list separated by ; is %s\n", token);
        cmd_buffer[num_cmd] = token;
        num_cmd++;
        token = strtok(NULL, s);
    }
    char** cmd_list = malloc(sizeof(char*) * (num_cmd));
    for (int i = 0; i < num_cmd; i++) {
        cmd_list[i] = cmd_buffer[i];
    }
    if (num_cmd == 0)
        last_empty = 1;
    if (!all_empty) {
        //printf("not all empty\n");
        myPrint(original_cmds);
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
        if (empty_space(token)) {
            token = strtok(NULL, s);
            continue; //jump directly to next loop
        }
        if (token[strlen(token) - 1] == '\n') { //deal with newline character
            token[strlen(token) - 1] = '\0';
        }
        //printf("token in a single command is %s\n", token);
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

//preprocessing with string
//normally, return 1 if two string are the same, 0 if not
//in cd's case, return 1 if str1 is cd exactly, return 2 if str1 is cdxxx
int same_str(char* str1, char* str2) {
    if (!(strncmp((const char *)str2, cd_str, sizeof(str2)))) { //str2 is cd
        if (!( strncmp((const char *)str1, (const char *)str2, sizeof(str1)) )) {//this IS CD
            return 2;
        } else if (!( strncmp((const char *)str1, (const char *)str2, 2) )) { //cd sth
            return 1;
        } else {
            return 0;
        }
    }
    if (!(strncmp((const char *)str1, (const char *)str2, sizeof(str1)))) {//two string the same 
        //printf("%s\n", str1);
        return 1;
    }
    return 0;
}

int handle_cd(char** arg_list, char** path) {
    int i = 0;
    if (!same_str(arg_list[0], cd_str)) //not cd
        return 0;

    if (arg_list[1] == NULL) { //argv[1] is empty
        if (same_str(arg_list[0], cd_str) == 2){ //only a single cd, change to main
            return 1;
        }
        else { //cdxx, extract xx
            *path = arg_list[0] + 2;
            }
    } else { //path in arg_list[1]
        *path = arg_list[1];
    }
    return 2; 
}

//determine if is a valid redirection, return 0 if sth wrong, return 1 if is valid redirection, 2 if advanced redirection
//arg will be altered if 1 and 2
//sth wrong: 
//int redirection_sign(char** arg_list) {

//}

//return 1 if wrong format of built in: exit >xxx pwd etc.
//will print err if wrong format
int wrong_builtin(char** arg_list) { //flag == 1 if exit, flag == 2 if pwd
    //printf("strlen of %s is %d\n", arg_list[0], strlen(arg_list[0]));
    if (!strncmp(exit_str, arg_list[0], 4)) { //exit
        //printf("oh this is definitely exit \n");
        if((strlen(arg_list[0]) > 4) || (arg_list[1] != NULL)){
            rais_err();
            return 1;
        }
        return 0;
    } else if (!strncmp(pwd_str, arg_list[0], 3)) { //pwd
        //printf("oh this is definitely pwd \n");
        if((strlen(arg_list[0]) > 3) || (arg_list[1] != NULL))  {
            rais_err();
            return 1;
        }
        return 0;
    } else {
        return 0;
    }
}

void execute_command(char** arg_list, char* a_cmd) { //execute a single command
    pid_t pid;
    int status;
    pid = fork();
    if (pid == 0) { //child
        if (wrong_builtin(arg_list))
            exit(0);
        //printf("enter child process\n");
        //int redir_sign = redirection_sign(arg_list);
        if (same_str(arg_list[0], pwd_str)) { //pwd
            char buff[PATH_MAX]; //two extra situation: pwd > xxx(this will be considered as pwd); pwd>xxx(this will go to execution)
            getcwd(buff, sizeof(buff));
            //myPrint("pwd\n");
            myPrint(buff);
            myPrint("\n");
        } else {
            if (execvp(arg_list[0], arg_list) == -1) 
                rais_err();
        }
        exit(0);
    } else {
        waitpid(pid, &status, 0);
        //printf("Child exited\n");
    }
    return;
}

int dir_x_exit(char* path) { //return 1 if a directory doesn't exist
    struct stat s;
    if (!stat(path, &s))
        return 0;
    return 1;
}




int main(int argc, char *argv[]) 
{
    int batch_mode = 0;
    FILE * fp = stdin;
    if (argc != 1) { //should be in batch mode
        batch_mode = 1;
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            rais_err();
            exit(0);
        }
    }
    
    char cmd_buff[MAX_BYTE + 1]; //initiate
    char *pinput;
    while (1) {
        if (!batch_mode) 
            myPrint("myshell> ");
        memset(cmd_buff, '\0', (MAX_BYTE + 1));
        if (fgets(cmd_buff, (MAX_BYTE + 1), fp) == NULL) {
            break;
        }

        //printf("\ncmd_buff: %c, %c, %c, %c, %c, %c\n", cmd_buff[0], cmd_buff[1], cmd_buff[2], cmd_buff[3], cmd_buff[4], cmd_buff[5]);
        //printf("\ncmd_buff: %d, %d, %d, %d, %d, %d\n", cmd_buff[0], cmd_buff[1], cmd_buff[2], cmd_buff[3], cmd_buff[4], cmd_buff[5]);
    
        if (handle_too_long_cmd(cmd_buff, fp)) {//too long
            rais_err();
            last_empty = 1;
            continue;
        }

        char** cmd_list = create_cmd_list(cmd_buff);
        for (int i = 0; i < num_cmd; i++) {
            int cd;
            char* path;
            //printf("cmd we got is %s\n", cmd_list[i]);
            char** arg_list = create_arg_list(cmd_list[i]);
            //printf("first and second arg we got is %s, %s\n", arg_list[0], arg_list[1]);
            if (same_str(arg_list[0], exit_str))  { //exit
                if (wrong_builtin(arg_list))
                    continue;
                //myPrint("exit\n");
                exit(0);
            } else if ( ( cd = handle_cd(arg_list, &path) ) ) { //cd
                if (cd == 1) { //only a single cd, change to main
                    //myPrint("cd\n");
                    chdir(getenv("HOME"));
                } else {
                    //myPrint("cd ");
                    //myPrint(path);
                    //myPrint("\n");
                    if (dir_x_exit(path) || (arg_list[2] != NULL)) { //check if directory exist;
                        rais_err();
                        continue;
                    } else {
                        chdir(path);
                    }
                }
            } else { //normal command
                last_empty = 0;
                execute_command(arg_list, cmd_list[i]);
            }
            free(arg_list);
        }
        free(cmd_list);
    }
}

