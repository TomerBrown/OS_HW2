#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

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
void print_array(int count ,char** array){
    int i =0;
    printf("[");
    for (i =0; i<count;i++){
        i != count-1 ? printf("%s ,", array[i]) :printf("%s]\n", array[i]) ;
    }
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

/*
*  A function that parses and changes the command according to the given arguments
*  When neccessary makes changes to the arglist or count parameters given to it as input
*/
Command arglistToCommand(int* count , char** arglist){
    Command command =  initCommand();
    int c = *count;
    if (c!=0){
        command.command = arglist[0];
    }
    int i = 0;
    for (i =0 ; i<c; i++){
        if (strcmp(arglist[i],"&")==0){
            // If & (run in background sign) found
            arglist[i] = NULL;
            *count = c-1;
            command.background = 1;
        }
        else if (strcmp(arglist[i],">")==0){
            // If output redirect was given to command

            //Firstly: update Command struct
            command.output_bol = 1;
            command.redictOutPath = arglist[i+1];

            //Secondly : Delete values from arglist
            arglist[i] = NULL;
            arglist [i+1] = NULL;

            //Thirdly : Update count
            *count = *count -2;
            return command;
        }
        else if (strcmp(arglist[i],"|")==0){
            // If pipe is given to command
            command.piping_bol = 1;
        }
    }
    
    return command;
}


void handle_SIGCHLD(int sig){
    /* When getting a SIGCHILD signal => wait for it so won't be zombied*/
    wait(NULL);
}

int prepare(){
    //Igonre SIGINT in shell it self
    signal(SIGINT, SIG_IGN);
    //When child finish wait for it so won't become zombie :_)
    signal(SIGCHLD,handle_SIGCHLD);
    return 0;
}

int finalize (){
    // Return SIGINT to default
    signal(SIGINT, SIG_DFL);
    //Kill Every process running if exited the shell (even if in background)
    kill(0,SIGKILL);
    return 0;
}

int process_arglist(int count, char **arglist){
    Command command = arglistToCommand(&count,arglist);
    int status;
    int file = 0;
    if (command.output_bol==1){
        //open the file (Or create if does not excists yet with correct permissions)
        file = open(command.redictOutPath,O_WRONLY | O_CREAT  ,0777);
        if (file < 0 ){
            //Error opening file
            fprintf(stderr, "Error: an error occured while opened a file\n");
            exit(1);
        }
    }

    int pid = fork();
    if (pid == 0){
        //Child

        //Deal with process that do not run on background
        if (command.background == 0){
            // as shell process is defaulted to Ignore sigint. regular processes should be defaulted
            signal(SIGINT,SIG_DFL);
        }

        //Deal with processes that should redirect their output to file
        if (command.output_bol == 1){
            //Redirect output of stdout (1) to file
            if (dup2(file,1)== -1){
                fprintf(stderr, "Error: an error occured while redirecting std out\n");
                exit(1);
            }
        }
        //Execute the process or report an error
        if (execvp(command.command,arglist)== -1){
            fprintf(stderr, "Error: Couldn't run process (Maybe name was incorrect) \n");
            exit(0);
        }

    }
    else{
        //Parent (part to execute)
        signal(SIGINT, SIG_IGN);
        if (command.background==0){
            // If the process should run regulary 
            waitpid(pid,&status,0);
            // after child process finish close file in was opened
            if (command.output_bol == 1){
                if (close(file)==-1){
                    fprintf(stderr, "Error: an error occured while closing file");
                }
            }
        }
    }
    return 1;
}