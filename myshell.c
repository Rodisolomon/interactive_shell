#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

int num_arg; //num of argument
int num_cmd;
int MAX_BYTE = 513;
char* exit_str = "exit\0";
char* cd_str = "cd\0";
char* pwd_str = "pwd\0";
char* rd_sign = ">";
char* advrd_sign = ">+";
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
    int all_empty;
    char original_cmds[514];
    strcpy(original_cmds, cmd_buff); //方便print if not empty
    //printf("cmd_list before is : %s\n", original_cmds);
    //separate command, create command list
    const char s[2] = ";";
    if (strstr(cmd_buff, s) == NULL)
        all_empty = 1;
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


char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

char* remove_white_space(char* single_cmd) {
    while(isspace((char)*single_cmd)) {
        single_cmd++;
    }
    if (!(*single_cmd))
        return single_cmd;
    char* end;
    end = single_cmd - 1 + strlen(single_cmd);
    while(isspace((char)*end) && (end > single_cmd)) {
        end--;
    }
    end[1] = '\0';
    return single_cmd;
}

char** create_arg_list(char* single_cmd) {
    //separate argument, create argument list for execvp
    num_arg = 0;
    single_cmd = remove_white_space(single_cmd);
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

int preprocess_redirect(char* cmd, int which_sign, char** arg, char** file) {
    //return 0 if sth wrong, lack of a&b in (a > b), also give back presumed argument and file
    const char* s;
    if (which_sign == 1) {
        s = ">";
    } else {
        s = ">+";
    }
    char* block_buf[20];
    char* block;
    int num_block = 0;
    block = strtok(cmd, s);
    while (block != NULL) {
        if (empty_space(block)) {
            block = strtok(NULL, s);
            continue; //jump directly to next loop
        }
        block_buf[num_block] = block; 
        num_block++;
        if (block[strlen(block) - 1] == '\n') { //deal with newline character
            block[strlen(block) - 1] = '\0';
        }
        block = strtok(NULL, s);
    }     
    if (num_block != 2)
        return 0;

    *arg = block_buf[0];
    *file = block_buf[1];
    return 1;


}
int multi_file(char* file) { //check file portion of redirection, if multiple return 0
    char** file_ls = create_arg_list(file);
    if (file_ls[1] != NULL)
        return 1;
    return 0;
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
int redirection_sign(char* cmd) {//return 1 if >; 2 if >+; 0 if no
    if (strstr(cmd, advrd_sign) != NULL) {
        return 2;
    }
    else if (strstr(cmd, rd_sign) != NULL) {
        return 1;
    } else {
        return 0;
    }
}

//redirection不好的case
int multiple_rdsign(char* cmd, char* checked_sign) { //return 1 if multiple signs
    int count = 0;
    const char *new = cmd;
    while( (new = strstr(new, (const char*)checked_sign)) ){
        count++;
        new++;
    }
    if (count > 1)
        return 1;
    return 0;
}

int multiple_rd(char* cmd) { // return 1 if eg. >> or >+>+
    char* block1;
    char* block2;
    int num_block = 0;
    char* block_buffer[520];
    if (redirection_sign(cmd) == 2) {
        const char s[2] = ">";
        if ( multiple_rdsign(cmd, s) )
            return 1;
        return 0;   
    } else if (redirection_sign(cmd) == 1) { //== 1
        const char s[2] = ">";
        if ( multiple_rdsign(cmd, s) )
            return 1;
    } 
    return 0;
}



//return 1 if wrong format of built in: exit >xxx pwd etc.
//will print err if wrong format
int wrong_builtin(char** arg_list) { //flag == 1 if exit, flag == 2 if pwd
    //printf("strlen of %s is %d\n", arg_list[0], strlen(arg_list[0]));
    if (!strncmp(exit_str, arg_list[0], 4)) { //exit
        //printf("oh this is definitely exit \n");
        //printf("%s\n", arg_list[1]);
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
    } else if (!strncmp(cd_str, arg_list[0], 2)) { //cd
        //printf("oh this is definitely pwd \n");
        if((strlen(arg_list[0]) > 2) || (arg_list[1] != NULL))  {
            rais_err();
            return 1;
        }
    } else {
        return 0;
    }
}
void execute_rd_command(char** arg_list, char* a_cmd, char* file, int rd_sign) {
    pid_t pid;
    pid_t pid2;
    int status2;
    int status;
    pid = fork();
    file = trimwhitespace(file);
    if (pid == 0) { //child
        if (wrong_builtin(arg_list))
            exit(0);
        //printf("enter child process\n");
        //int redir_sign = redirection_sign(arg_list);
        //printf("   arg_list[0]   is %s\n", arg_list[0]);
        if (same_str(pwd_str, arg_list[0])) { //pwd
            rais_err();
            exit(0);
        } else if (same_str(cd_str, arg_list[0]) || same_str(exit_str, arg_list[0])){
            rais_err();
            exit(0);
        } else {
            if (rd_sign == 1) { //redirection
                mode_t mode = O_RDWR | O_CREAT | O_EXCL;
                int new_fd = open(file, O_WRONLY | O_CREAT | O_EXCL, 0664); 
                if (new_fd > 0) {
                    dup2(new_fd, STDOUT_FILENO);
                    if (execvp(arg_list[0], arg_list) == -1) {
                        rais_err();
                    }
                } else {
                    rais_err();
                }
            close(new_fd);
            exit(0);
            } else { //super redirection
                printf("super redirection!\n");
                mode_t mode = O_WRONLY | O_CREAT | O_APPEND;
                char* temp_f = "temp_f";
                int tem_fd = open(temp_f, mode, 0664);
                if (tem_fd > 0) {
                    dup2(tem_fd, STDOUT_FILENO);
                    pid2 = fork();
                    if (pid2 == 0) { //child的child
                        printf("enter child child process\n");
                        if (execvp(arg_list[0], arg_list) == -1) 
                            rais_err();   
                        exit(0);      
                    } else { //child的parent
                        waitpid(pid2, &status2, 0);
                        printf("exit child child process\n");
                        int old_fd = open(file, O_RDWR, 0664);
                        char tem_buf[2];
                        int num;
                        while ((num = read(old_fd, tem_buf, 2))) {
                            write(STDOUT_FILENO, tem_buf, num);
                        }
                        rename("temp_f", file);
                        close(tem_fd);
                        close(file);
                    }
                    
                } else{
                    rais_err();
                    exit(0);
                }                

            }
        }
    } else {
        waitpid(pid, &status, 0);
        //printf("Child exited\n");
    }
    return;
}

void execute_command(char** arg_list, char* a_cm) { //execute a single command
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
    DIR* dir = opendir(path);
    if (dir) {
        closedir(dir);
        return 0;
    } else {
        return 1;
    }
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

        if (handle_too_long_cmd(cmd_buff, fp)) {//handle too long
            rais_err();
            last_empty = 1;
            continue;
        }
        char** rd_arg_list;
        char** cmd_list = create_cmd_list(cmd_buff);
        for (int i = 0; i < num_cmd; i++) {
            int cd;
            char* path;
            char* arg;
            char* file;

            ///handle redirection ////
            int rd_sign = redirection_sign(cmd_list[i]);
            //printf("rd_sign is %d\n", rd_sign);
            if (rd_sign) { //possible redirection
                //printf("this is rd!!\n");
                if (multiple_rd(cmd_list[i])) {
                    rais_err();
                    continue;
                }
                if (! preprocess_redirect(cmd_list[i], rd_sign, &arg, &file)) {
                    rais_err();
                    continue;
                }
                if (multi_file(file)) {
                    rais_err();
                    continue;
                }
                rd_arg_list = create_arg_list(arg);
            }




            char** arg_list = create_arg_list(cmd_list[i]);
            //printf("cmd we got is %s\n", cmd_list[i]);
            //printf("first and second arg we got is %s, %s\n", arg_list[0], arg_list[1]);

            if (same_str(exit_str, arg_list[0]))  { //exit
                if (wrong_builtin(arg_list))
                    continue;
                if (rd_sign == 1) {
                    rais_err();
                    continue;
                }

                //myPrint("exit\n");
                exit(0);
            } else if ( ( cd = handle_cd(arg_list, &path) ) ) { //cd
                //printf("a cd command %d\n", cd);
                if (rd_sign) {
                    rais_err();
                    continue;
                } else if (cd == 1) { //only a single cd, change to main
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
                if (!rd_sign) {//rd = 0
                    //printf("not rd!\n");
                    execute_command(arg_list, cmd_list[i]);
                } else {
                    //printf("rd!\n");
                    execute_rd_command(rd_arg_list, cmd_list[i], file, rd_sign);
                }
            }
            free(arg_list);
        }
        free(cmd_list);
    }
}

