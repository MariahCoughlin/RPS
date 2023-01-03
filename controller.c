#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include "controller.h"
#include <errno.h>
#include <stddef.h>
#include <sys/wait.h>
#include <stdbool.h>

void initialize(int pipes[][NUMPIPES], pid_t idList[]);
void child(int respondingPipe[]);
void readyHandler(int sig);
void setupHandler();
void playGame(int numRounds, int pipeList[][NUMPIPES], pid_t idList[]);
int evaluate(char player0, char player1);
void printResults(int wins[]);

int main(int argc, char** argv){
    int numRounds, i, err;
    pid_t childId[NUMPLAYERS];
    int pipeList[NUMPLAYERS][NUM_PIPE_ENDS];
    
    if(argc != 2){
        printf("Error, invalid number of arguments. Required: 1\n");
        exit(1);
    }//end if
    numRounds = atoi(argv[1]);
    
    initialize(pipeList, childId);

   
    for(i = 0; i < NUMPLAYERS; i++)
        pause();
    // Once both are ready, loop until enough games have been played
    playGame(numRounds, pipeList, childId);
    
    printf("Controller:     terminating\n");
    err = sleep(1); //Prevent formatting issues in terminal
    if(err < 0){
        printf("Controller: Error in sleep - %d\n", errno);
        exit(21);
    }//end if
    return 0;
}//endmain



//-------initialize------------------------
// Called when the controller is started.
// Sets up signal handler so player can
// send controller signals
//----------------------------------------
void initialize(int pipes[][NUMPIPES], pid_t idList[]){
    int err, i;
    setupHandler();
   
   //------Create the pipes --------------
    for(i = 0; i < NUMPIPES; i++){
        err = pipe(pipes[i]);
        if(err < 0){
            printf("Error in creating pipe%d; Terminating.\n", i);
            exit(2);
        }//end if
    }//endfor
    
    //-----Create the child processes-------------
    for(i = 0; i < NUMPLAYERS; i++){
        idList[i] = fork();
        if(idList[i] < 0){
            printf("Error in forking child %d, terminating\n", i);
            exit(3);
        }//endif
        
        if(idList[i] == 0)
            child(pipes[i]);
    }//endfor
    
   //close both ends of pipes
    for(i = 0; i < NUMPLAYERS; i++)
        close(pipes[i][WRITE]);
    
}//endinitialize


//-------------Child-----------------------
// The child process forked in initialize()
// is sent here. Executes player process
//-----------------------------------------
void child(int respondingPipe[]){
    int err;
    char* paramVector[4];
    char resPipeBuffer[11];
    
    //close end of pipe
    err = close(respondingPipe[READ]);
    if(err < 0){
        printf("Controller: Error in closing read end of pipe in child: %d\n", errno);
        exit(18);
    }//end if
    
    // Cast the pipe id to strings
    sprintf(resPipeBuffer, "%d%c", respondingPipe[WRITE], NULL);
    
    //Parameter 0: Program Name
    paramVector[0] = PLAYERPROCESSNAME;
    //Parameter 1: Pipe
    paramVector[1] = resPipeBuffer;
    //Parameter 3: Null
    paramVector[2] = NULL;
    err = execve("./Player", paramVector, NULL);
    if(err < 0){
        printf("Error in executing a player process. Terminating\n");
        exit(19);
    }//endif
}//endchild

/****************************************************************************************
    do_game
    Simulates RPS for the designated number of rounds.
    Parameters:
                * int numRounds - the number of rounds.
    Returns:
                * N/A
*****************************************************************************************/
void playGame(int numRounds, int pipeList[][NUMPIPES], pid_t idList[]){
    int roundCount = 0, i, err, winner;
    char results[2][MAXBUFFERSIZE];
    int wins[NUMPLAYERS + 1];
    for(i = 0; i < NUMPLAYERS + 1; i++)
        wins[i] = 0;
    
    while(roundCount < numRounds){
            //Signal show
            for(i = 0; i < NUMPLAYERS; i++){
                printf("Controller:     sending reading command\n");
                err = kill(idList[i], SIGUSR1);
                if(err < 0){
                    printf("Controller: Error in sending Show to player - %d\n", errno);
                    exit(12);
                }//endif
            }//endfor
                
            //Wait for players to put choice into pipe.
            err = sleep(1);
            if(err < 0){
                printf("Controller: Error in sleep - %d\n", errno);
                exit(20);
            }//endif
            
            //Get results from pipes
            for(i = 0; i < NUMPLAYERS; i++)
                read(pipeList[i][READ], results[i], MAXBUFFERSIZE);
        
            //Do logic with results
            winner = evaluate(results[0][0], results[1][0]);
            wins[winner + 1]++;
            
            ++roundCount;
    }//endwhile
    
    printResults(wins);
    
    for(i = 0; i < NUMPLAYERS; i++){
        printf("Controller:     sending done command\n");
        err = kill(idList[i], SIGUSR2);
        if(err < 0){
            printf("Controller: Error in sending Done to player - %d\n", errno);
            exit(13);
        }//endif
    }//endfor
}//endplayGame


//-----------evaluate--------------------
// Evaluates the winner
//----------------------------------------
int evaluate(char player0, char player1){
    int result;
    if(player1 % player0){ //is not a draw
        if(  ( (NUMCHOICES + player0 - player1) % NUMCHOICES) % 2)
            result = 1;
        else
            result = 0;
        
        printf("***Player %d wins!***\n\n", result);
    }//endif
    else{ //is a draw
        result = -1;
        printf("The match was a draw!\n\n");
    }//end else
    return result;
}//endevaluate


//-------readyHandler----------------
// Signal handler for ready signal
//----------------------------------
void readyHandler(int sig){
    printf("Controller:     Received ready\n");
}//endreadyHandler


//----setUpHandler---------
// sets up ready signal
//--------------------------
void setupHandler(){
    struct sigaction ready_act;
    int err;
    
    sigset_t mask;
    sigemptyset(&mask);
    
    ready_act.sa_handler = (void(*)(int))readyHandler;
    ready_act.sa_mask = mask;
    ready_act.sa_flags = 0;
    err = sigaction(SIGUSR1, &ready_act, NULL);
    if(err < 0){
        printf("controllerSignal: Error %d on sigaction.\n", errno);
        exit(5);
    }//endif
}//endsetupHandler


//-----printResults-----------
//Prints the final results
//----------------------------
void printResults(int wins[]){
    printf("Finished\nDraws: %d\n***Player0 wins: %d***\n***Player1 wins: %d***\n", wins[0], wins[1], wins[2]);
}//endprintResults
