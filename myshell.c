#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
# include <errno.h>

/*A struct to parse the command to diffrent part and info needed*/ 
typedef struct Command {
    char* command; /*1) The file name to execute*/
    int background; /* 2) boolean to run in background or not*/
    int output_bol; /* 3) A boolean to indicate if redirect or not*/
    char* redictOutPath; /*4) The FileName to redirect to*/
    int piping_bol; /*5) boolean to indicate if needs pipe*/
    char* command1; /*6) the name of the program to run (redirect output)*/
    char* command2; /*7) the name of the program that recieves output*/
    char** arglist1;/*8) The array of arguments for program 1*/
    int count1; /*9) The length of the array of arguments for program 1*/
    char** arglist2; /*10) The array of arguments for program 2*/
    int count2; /*11) The length of the array of arguments for program 2*/

} Command;

/*******************************************
 ********* Function Declerations ***********
 ******************************************/

/* A function that print in array in format [x,y,...,z] */
void print_array(int,char**);

/*A function that prints out a command (in a parsed form) */
void printCommand(Command*);


/* When getting a SIGCHILD signal => wait for it so won't be zombied*/
void handle_SIGCHLD(int);

/* A function that parses and changes the command according to the given arguments
*  When neccessary makes changes to the arglist or count parameters given to it as input */
Command arglistToCommand(int*, char**);


/*******************************************
 *********** Helper Functions **************
 ******************************************/

/* A function that initializes the values to be or NULL*/
Command initCommand(){
    Command command;
    command.command = NULL;
    command.background = 0;
    command.output_bol = 0;
    command.redictOutPath = NULL;
    command.piping_bol = 0;
    command.arglist1 = NULL;
    command.count1 = 0;
    command.arglist2 = NULL;
    command.count2 = 0;
    command.command1 = NULL;
    command.command2 = NULL;
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
    printf("\tpiping_bol: %d \n", cmd->piping_bol);
    printf("\tcommand 1: %s \n", cmd->command1);
    printf("\tcommand 2: %s \n", cmd->command2);
    printf("\tcount1 %d \n", cmd->count1);
    printf("\targlist1: ");
    print_array(cmd->count1,cmd->arglist1);
    printf("\tcount2 %d \n", cmd->count2);
    printf("\targlist2: ");
    print_array(cmd->count2,cmd->arglist2);
    printf("}\n");
}

void handle_SIGCHLD(int sig){
    wait(NULL);  
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
            arglist[i] = NULL;
            command.arglist1 = arglist;
            command.count1 = i;
            command.arglist2 = arglist + i +1;
            command.count2 = c - i -1;
            if (i>0 && i <c-1){
                command.command1 = command.arglist1[0];
                command.command2 = command.arglist2[0];
            }
        }
    }
    
    return command;
}


/*******************************************
 ********** Requested Functions ************
 ******************************************/

/* A function to run before any command has start*/
int prepare(){
    //Igonre SIGINT in shell it self
    signal(SIGINT, SIG_IGN);
    struct sigaction sa;
    sa.sa_handler = handle_SIGCHLD;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);
    return 0;
}

/* A function to run after shell is closed (Used to kill all child processes)*/
int finalize (){
    // Return SIGINT to default
    signal(SIGINT, SIG_DFL);
    //Kill Every process running if exited the shell (even if in background)
    if (kill(0,SIGKILL)==-1){
        return 1;
    };
    return 0;
}

/* The Function to execute the command the given by shell.c file*/
int process_arglist(int count, char **arglist){
    //Initialize parameters
    int status;
    int status2;
    int pipe_fd[2];
    int pid2;
    int file = 0;

    // Parse the command to diffrent parts that will be used later
    Command command = arglistToCommand(&count,arglist);

    if (command.output_bol==1){
        //open the file (Or create if does not excists yet with correct permissions)
        file = open(command.redictOutPath,O_WRONLY | O_CREAT  ,0777);
        if (file < 0 ){
            //Error opening file
            fprintf(stderr, "Error: an error occured while opened a file\n");
            return 0;
        }
    }
    
    //Fork to execute the command in a diffrent process
    int pid = fork();
    if (pid < 0){
        fprintf(stderr, "Error: an error occured while forking a pipe. please try again");
    }

    else if (pid == 0){
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
        if (command.piping_bol==1){
            if (pipe(pipe_fd) == -1){
                fprintf(stderr, "Error: an error occured while trying to create a pipe");
                exit(1);
            }
            pid2 = fork();
            //We will assume that child passes inforamtion through the pipe to parent 
            if (pid2 < 0 ){
                fprintf(stderr, "Error: an error occured while forking a pipe");
                exit(1);
            }
            else if (pid2 == 0){
                //Child proccess
                //Change stdout to write output to pipe (write port)
                dup2(pipe_fd[1],1);
                command.command = command.command1;
            }
            else{
                //Parent
                //Wait for child to finish and Change stdin to recieve input from pipe (read port)
                wait(&status);
                dup2(pipe_fd[0],0);
                command.command = command.command2;
            }
        }

        //Execute the process or report an error
        if (execvp(command.command,arglist)== -1){
            fprintf(stderr, "Error: Couldn't run process (Maybe name was incorrect) \n");
            exit(1);
        }
    }
    else{
        //Parent (part to execute)
        signal(SIGINT, SIG_IGN);
        if (command.background==0){
            // If the process should run regulary - wait for it
            if (waitpid(-1,&status2,0)== -1){
                fprintf(stderr, "Error: an error occured while waiting");
                if (errno == ECHILD || errno == EINTR){
                    fprintf(stderr, "Error: an error occured while waiting");
                    return 0;
                }
            }
            
            // after child process finish close file in was opened
            if (command.output_bol == 1){
                if (close(file)==-1){
                    fprintf(stderr, "Error: an error occured while closing file");
                    return 0;
                }
            }
        }
    }
    return 1;
}
