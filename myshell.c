#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

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


Command arglistToCommand(int count , char** arglist){
    Command command =  initCommand();
    if (count!=0){
        command.command = arglist[0];
    }
    int i = 0;
    for (i =0 ; i<count; i++){
        if (strcmp(arglist[i],"&")==0){
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
    return 0;
}

int finalize (){
    return 0;
}


void print_array(int count ,char** array){
    int i =0;
    printf("[");
    for (i =0; i<count;i++){
        i != count-1 ? printf("%s ,", array[i]) :printf("%s]\n", array[i]) ;
    }
}

int process_arglist(int count, char **arglist){
    int status;
    int pid = fork();
    Command command = arglistToCommand(count,arglist);
    if (pid == 0){
        //Child (part to execute)
        execvp(command.command,arglist);
    }
    else{
        if (command.background==0){
            waitpid(pid,&status,0);
        }   
    }
    return 1;
}