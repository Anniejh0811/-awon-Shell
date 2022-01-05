#include "stdio.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/fcntl.h>

enum SWITCH_CASE
{
    LOOP = 0,
    PATH,
    RUN_PROCESS,
    EXIT,
    CD
};


#define MAX_COMMAND_LEN (10)
#define MAX_PATHS (100)
#define MAX_PATH_LEN (200)
char **paths;
int num_paths = 0;
const char error_message[30] = "An error has occurred\n";
FILE *output = NULL;

int run_process(char **tokens, int tonken);
void command(char *cmd, size_t count);

void print_error(void){
    write(STDERR_FILENO, error_message, strlen(error_message));
}
void add_path(char *path)
{
    num_paths++;
    paths = realloc(paths, sizeof(char *)*num_paths); // assign one space to paths
    paths[num_paths-1] = malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(paths[num_paths-1], path);

}

int is_number(char* num){
    int index = 0;
    while(num[index]!='\0'){
        if(isdigit(num[index])==0){
            return 0;
        }
        index++;
    }
    return 1;
}

int batch(char *filename) {
    FILE *fptr = fopen(filename, "r");
    if (fptr == NULL){
        return 1;
    }

    char *cmd = malloc(sizeof(char) * MAX_COMMAND_LEN);
    size_t bufsize = MAX_COMMAND_LEN;
    size_t count;

    while ((count = getline(&cmd, &bufsize, fptr))!=-1) {
        cmd[count-1] = '\0';
        command(cmd, count);
    }
    fclose(fptr);
}

int run_process(char **tokens, int num_tokens)
{
    int status = 0;
    char * str;
    int ch = 0;
    int p = 0;

    // try all paths
    for (int i = 0; i < num_paths; i++) {
        str = malloc(sizeof(tokens[0])+sizeof(paths[i])+sizeof(char));
        strcpy(str, paths[i]);
        strcat(str, "/");
        strcat(str, tokens[0]);
        i = p;

        if (access(str, X_OK) == 0) {
            ch = 1;
            break;
        } else {

        }
    }


    if(ch == 0){
        print_error();
        return -1;
    }



    pid_t pid = fork();
    if(pid == 0){
        for(int i = 0; i<num_tokens; i++){
            if(strcmp(tokens[i], ">")==0){
                int fd = open(tokens[i+1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

                dup2(fd, 1);  
                dup2(fd, 2);  

                close(fd);
                tokens = (char **)realloc(tokens, sizeof(char*)*(i+1));
                tokens[i] = NULL;
                break;
            }
        }
        if(execv(str, tokens) == -1){
            print_error();
        }

    }else if(pid == -1){
        //error
        print_error();
    }else{
        //parent process
        wait(&status);
    }
    free(str);
    return status;
}


void command(char *cmd, size_t count){
    char *lineBuffer = NULL;
    size_t lineBufferSize = 0;
    char *string, *tok, *tok1;
    char **args;
    int switch_case = 0;

    string = cmd;
    // command parse
    int num_tokens_num = 2;
    for(int i = 0; i<count; i++) {
        if(isspace(cmd[i]))
            num_tokens_num++;
    }
    char **tokens = malloc(sizeof(char*) * num_tokens_num);
    char **cp_tokens = malloc(sizeof(char*) * num_tokens_num);
    int index = 0;
    while ((tok = strsep(&string, " \f\n\r\t\v")) != NULL) {
        if (strcmp(tok, "") != 0) {
            tokens[index++] = tok;

        }
    }

    tokens[index] = NULL;

    if (strcmp(tokens[0], "loop") == 0 && is_number(tokens[1]))
        switch_case = LOOP;
    else if (strcmp(tokens[0], "path") == 0)
        switch_case = PATH;
    else if (strcmp(tokens[0], "exit") == 0)
        switch_case = EXIT;
    else if (strcmp(tokens[0], "cd") == 0)
        switch_case = CD;
    else
        switch_case = RUN_PROCESS;
    switch (switch_case) {
        case LOOP:
        {

            int num_loops = atoi(tokens[1]);
            char **nested_tokens = tokens + 2;
            int *loop_variable_index = malloc(sizeof(int*)*num_tokens_num);
            int loop_variable_num = 0;

            for (int j = 0; j < num_tokens_num-1; j++) {
                if (strcmp(tokens[j], "$loop") == 0) {

                    loop_variable_index[loop_variable_num] = j;
                    loop_variable_num++;


                }
            }

            for (int i = 0; i < num_loops; i++) {
                for(int j = 0; j<loop_variable_num; j++){
                    sprintf(tokens[loop_variable_index[j]], "%d", i+1);
                }
                run_process(nested_tokens, index-2);
            }
            free(loop_variable_index);
            break;
        }
        case PATH:
            num_paths=0;
            for (int i = 1; i < index; i++) {

                add_path(tokens[i]);
            }
            break;
        case CD:
        {
            int ret = 0;
            ret = chdir(tokens[1]);
            if (ret!=0)
                print_error();
        }
            break;
        case EXIT:
            exit(0);
            break;
        case RUN_PROCESS:
            run_process(tokens, index);
            break;
        default:
            print_error();
            break;
    }
}

void interactive(void)
{
    char *cmd = malloc(sizeof(char) * MAX_COMMAND_LEN);
    size_t bufsize = MAX_COMMAND_LEN;
    size_t count;


    while (1) {
        printf("awon>");
        // command read
        count = getline(&cmd, &bufsize, stdin);
        cmd[count - 1] = '\0';
        command(cmd, count);
    }
    free(cmd);
}


int main(int argc, char *argv[])
{

    paths = malloc(sizeof(char*));
    add_path("/bin");
    if (argc == 1) {
        interactive();
    } else if (argc == 2) {
        batch(argv[1]);
    } else {
        print_error();
    }
}
