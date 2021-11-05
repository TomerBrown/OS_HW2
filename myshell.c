#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>


typedef struct Command {
    char* command; /*The file name to execute*/
    int background; /* boolean to run in background or not*/
    int output_bol; /* A boolean to indicate if redirect or not*/
    char* redictOutPath; /*The FileName to redirect to*/
    int piping_bol; /*boolean to indicate if needs pipe*/
} Command;

Command initCommand(){
    Command command;
    command.command = NULL;
    command.background = 0;
    command.output_bol = 0;
    command.redictOutPath = NULL;
    command.piping_bol = 0;
    return command;
}


Command arglistToCommand(int* count , char** arglist){
    Command command =  initCommand();
    int c = *count;
    if (c!=0){
        command.command = arglist[0];
    }
    int i = 0;
    for (i =0 ; i<c; i++){
        if (strcmp(arglist[i],"&")==0){
            arglist[i] = NULL;
            *count = c-1;
            command.background = 1;
        }
        if (strcmp(arglist[i],">")==0){
            command.output_bol = 1;
            command.redictOutPath = arglist[i+1];
        }
        if (strcmp(arglist[i],"|")==0){
            command.piping_bol = 1;
        }
    }
    return command;
}

void printCommand (Command* cmd){
    printf("Command {\n");
    printf("\tcommand: %s \n", cmd->command);
    printf("\tbackground: %d \n", cmd->background);
    printf("\toutputbol: %d \n", cmd->output_bol);
    printf("\tredictOutPath: %s \n", cmd->redictOutPath);
    printf("\tpiping_bol %d \n", cmd->piping_bol);
    printf("}\n");
}


int prepare(){
    signal(SIGINT, SIG_IGN);
    return 0;
}

int finalize (){
    signal(SIGINT, SIG_DFL);
    kill(0,SIGKILL);
    return 0;
}

void handle_SIGCHLD(int sig){
    /* When getting a SIGCHILD signal => wait for it so won't be zombied*/
    wait(NULL);
}

void print_array(int count ,char** array){
    int i =0;
    printf("[");
    for (i =0; i<count;i++){
        i != count-1 ? printf("%s ,", array[i]) :printf("%s]\n", array[i]) ;
    }
}


int process_arglist(int count, char **arglist){
    Command command = arglistToCommand(&count,arglist);
    int status;
    int pid = fork();
    if (pid == 0){
        //Child
        if (command.background == 0){
            signal(SIGINT,SIG_DFL);
        }
        execvp(command.command,arglist);
    }
    else{
        //Parent (part to execute)
        signal(SIGINT, SIG_IGN);
        signal(SIGCHLD,handle_SIGCHLD);
        if (command.background==0){
            waitpid(pid,&status,0);
        }
    }
    return 1;
}