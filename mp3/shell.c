/** @file shell.c */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "log.h"


void parser(char *currchar, char** parsed)
{
    while(*currchar != '\0')
    {
        while(*currchar == ' ')
        {
            *currchar = '\0';
            currchar++;
        }
        *parsed = currchar; 
        parsed++;
        while(*currchar != ' ' && *currchar != '\0')
            currchar++;
    }
    *parsed = '\0';
}
/**
 * Starting point for shell.
 */
int main() {
    log_t* cmdlog = malloc(sizeof(log_t));
    log_init(cmdlog);
    size_t buffersize = 1;
    char* cwd = malloc(75);
    char* input = malloc(buffersize);
    int getinput = 1;
	while(1){
        int currpid = getpid();
        getcwd(cwd, 50);
        if(getinput)
        {
            printf("(pid=%d)%s$ ", currpid, cwd);
            getline(&input, &buffersize, stdin);
            input[strlen(input)-1] = '\0';
        }
        else
        {
            getinput = 1;
        }
        
        //printf("%s:end", input);
        if(strncmp(input, "cd ", 3)==0 || strcmp(input, "exit")==0 || strcmp(input, "!#")==0
            || strncmp(input, "!", 1)==0)
        {
            printf("Command executed by pid=%d\n", currpid);
            if(strncmp(input, "cd ", 3)==0){
                char* path = (char*)(input+3);
                if(chdir(path)==-1) 
                {
                    printf("%s: No such file or directory\n", path);
                }
                log_push(cmdlog, input);
            }
            else if(strcmp(input, "exit")==0) {
                log_destroy(cmdlog);
                free(cwd);
                free(input);
                return 0;
            }
            else if(strcmp(input, "!#")==0){
                int remain = cmdlog->start->size;
                log_t* curr = cmdlog->start;
                int i;
                while(remain > 0){
                    for(i=0; i<remain; i++)
                    {
                        curr = curr->next;
                    }
                    printf("%s\n", curr->item);
                    remain = remain-1;
                    curr = cmdlog->start;
                }
                /*
                log_t* revlog = malloc(sizeof(log_t));
                log_init(revlog);
                log_t* curr = cmdlog->start;
                while(curr->next){
                    curr = curr->next;
                    log_push(revlog, curr->item);
                }
                curr = revlog->start;
                while(curr->next){
                    curr = curr->next;
                    printf("%s\n", curr->item);
                }
                log_destroy(revlog);
                */
            }
            else if(strncmp(input, "!", 1)==0){
                char* last;
                char* prefix = (char*)(input+1);
                last = log_search(cmdlog, prefix);
                if(last == NULL)
                {
                    printf("No Match\n");
                }
                else
                {
                    printf("%s matches %s\n", prefix, last);
                    unsigned long i;
                    for(i=0; i<strlen(last)+1; i++)
                    {
                        input[i] = last[i];
                    }

                    getinput = 0;

                }
                
            }
        }
        else 
        {
            //system(input);
            pid_t childpid = 0;
            if((childpid = fork()))
            {
                printf("Command executed by pid=%d\n", childpid);
                waitpid(childpid, NULL, WUNTRACED);
            }
            else
            {
                int execret = 0;
                
                //char* bininput = malloc(5+strlen(input)+1);
                //strcpy(bininput, "/bin/");
                char* parsedin[50];
                char* currchar = input;
                parser(currchar, parsedin);
                
                execret = execvp(*parsedin, parsedin);
                if(execret == -1)
                {
                    printf("%s: not found\n", input);
                }
                log_destroy(cmdlog);
                free(cwd);
                free(input);
                return 0;
            }
            //printf("%d\n", *execret);
            log_push(cmdlog, input);
        }
    }
}

