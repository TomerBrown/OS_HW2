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

/*When needed => Wait for child so it won't become zombie*/
void handle_SIGCHLD(int sig){
    wait(NULL);
}

/*A function that parses the given command in to Command Struct
When necessary changes it's input: arglist so it will be compatible
to use when using execvp(command, arglist)*/
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

/*A function to set SIGCHLD handler to avoid zombies!*/
void set_sigaction_of_shell(Command* command){
    if (command->background){
        struct sigaction sa;
        sa.sa_handler = handle_SIGCHLD;
        sa.sa_flags = SA_RESTART;
        sigaction(SIGCHLD, &sa, NULL);
    }
    else{
        signal(SIGCHLD,SIG_IGN);
    }
    
    
}

/*A function to deal with the regular option - no special symbol inserted*/
int regular_process(Command* command , char** arglist){

    //Fork a new child
    pid_t pid = fork();
    int status;

    if (pid < 0){
        //Report On Error
        fprintf(stderr, "Error: an error occured while forking. please try again\n");
        return -1;
    }

    if (pid == 0){
        //Child Process 
        signal (SIGINT,SIG_DFL);
        if (execvp(command->command, arglist)== -1){
            fprintf(stderr, "Error: Couldn't run process (Maybe name was incorrect) \n");
            exit(1);
        }
    }

    // Shell Proccess
    if (waitpid(pid , &status, 0)==-1){
        if (errno != ECHILD && errno != EINTR){
            return 1;
        }
    }
    return 0;
}

/*A function that deals with processes that need to run in background*/
int run_on_background (Command* command, char** arglist){
    
    pid_t pid = fork ();
    if (pid < 0){
        //Report On Error
        fprintf(stderr, "Error: an error occured while forking. please try again\n");
        return -1;
    }
    if (pid ==0){
        //Child Process

        //Set to ignore SIGINT so ^C won't affect this process
        signal (SIGINT, SIG_IGN);
        if (execvp(command->command, arglist)== -1){
            fprintf(stderr, "Error: Couldn't run process (Maybe name was incorrect) \n");
            exit(1);
        }
    }
    
    // Shell - Not waiting for program and returning
    return 0;

}

/*A function that deals with processes that uses pipe*/
int piped_process (Command* command){

    //Initialize Parameters
    int pipe_fd [2];
    pid_t pid1 , pid2 = 0;

    //Initialize The Pipe
    if (pipe(pipe_fd) == -1){
        fprintf(stderr, "Error: an error occured while trying to create a pipe");
        exit(1);
    }

    pid1 = fork();
    if (pid1 < 0){
        //Report On Error
        fprintf(stderr, "Error: an error occured while forking. please try again\n");
        return -1;
    }
    if (pid1 == 0){
        // First Child Process (First Child -Writes-> to Second Child)

        //Firstly: Change stdout so will write to pipe
        close (pipe_fd[0]);
        dup2(pipe_fd[1],1);
        close(pipe_fd[1]);

        //Secondly: Assure that proccess will terminate on SIGINT
        signal(SIGINT,SIG_DFL);
        
        //Thirdly: Run the process
        if (execvp(command->command1,command->arglist1)== -1){
            fprintf(stderr, "Error: Couldn't run process (Maybe name was incorrect) \n");
            exit(1);
        }
    }

    pid2 = fork();
    if (pid2<0){
        //Report On Error
        fprintf(stderr, "Error: an error occured while forking. please try again\n");
        return -1;
    }
    
    if (pid2 == 0){
        // First Child Process (First Child -Writes-> to Second Child)

        //Firstly: Change stdin so will read from pipe
        close (pipe_fd[1]);
        dup2(pipe_fd[0],0);
        close(pipe_fd[0]);

        //Secondly: Assure that proccess will terminate on SIGINT
        signal(SIGINT,SIG_DFL);
        
        //Thirdly: Run the process
        if (execvp(command->command2,command->arglist2)== -1){
            fprintf(stderr, "Error: Couldn't run process (Maybe name was incorrect) \n");
            exit(1);
        }
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);

    if (waitpid(pid1 , NULL, 0)==-1){
        if (errno != ECHILD && errno != EINTR){
            return 1;
        }
    }
    if (waitpid(pid2 , NULL, 0)==-1){
        if (errno != ECHILD && errno != EINTR){
            return 1;
        }
    }
    return 0;
}

/*A function that deals with processes that redirect output to file*/
int output_redirect_process (Command* command , char** arglist){
    
    //Initialize Parameters
    int file = 0;

    //open the file (Or create if does not excists yet with correct permissions)
    file = open(command->redictOutPath,O_WRONLY | O_CREAT  ,0777);
    if (file < 0 ){
        //Error opening file
        fprintf(stderr, "Error: an error occured while opened a file\n");
        return 0;
    }
    
    //Fork a new child
    pid_t pid = fork();
    

    if (pid < 0){
        //Report On Error
        fprintf(stderr, "Error: an error occured while forking. please try again\n");
        return -1;
    }
    
    if (pid == 0){
        //Child Process

        //Redirect output of stdout (1) to file
        if (dup2(file,1)== -1){
            fprintf(stderr, "Error: an error occured while redirecting std out\n");
            exit(1);
        }
        if (close(file)==-1){
            fprintf(stderr, "Error: an error occured while closing file");
            return 0;
        }

        //Set Signal so would be interuptable
        signal (SIGINT,SIG_DFL);

        //Execute the program
        if (execvp(command->command, arglist)== -1){
            fprintf(stderr, "Error: Couldn't run process (Maybe name was incorrect) \n");
            exit(1);
        }

    }

    // Shell Proccess
    if (waitpid(pid , NULL, 0)==-1){
        if (errno != ECHILD && errno != EINTR){
            return 1;
        }
    }
    if (close(file)==-1){
        fprintf(stderr, "Error: an error occured while closing file");
        return 0;
    }
    return 0;
}


/*******************************************
 ********** Requested Functions ************
 ******************************************/

/* A function to run before any command has start*/
int prepare(){
    //Igonre SIGINT in shell it self
    signal(SIGINT, SIG_IGN);
    return 0;
}

/* A function to run after shell is closed (Used to kill all child processes)*/
int finalize (){
    // Return SIGINT to default
    signal(SIGINT, SIG_DFL);
    //Kill Every process running if exited the shell (even if in background)
    if (kill(0,SIGTERM)==-1){
        return 1;
    };
    return 0;
}

/* The Function to execute the command the given by shell.c file*/
int process_arglist(int count, char ** arglist){
    Command command = arglistToCommand(&count,arglist);
    set_sigaction_of_shell(&command);

    if (!command.output_bol && ! command.piping_bol && ! command.background){
        //If it is not any special case - use the regular option
        regular_process(&command, arglist);
    }

    if (command.background){
        //Need to run process on backgroung and that won't terminate on ^C (SIGINT)
        run_on_background(&command, arglist);
    }

    if (command.piping_bol){
        //Run command with piping
        piped_process (&command);
    }

    if (command.output_bol){
        output_redirect_process(&command,arglist);
    }
    return 1;
}
