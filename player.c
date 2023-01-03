//---------------------------------------------------
// Implementation for the players in the RPS game.
//---------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "controller.h"
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>

void showHandler(int sig);
void doneHandler(int sig);
void signalReady(pid_t parent);
void playingRPS();
void getChoice(char* buff);
void setupHandlers();

bool playing = true;
int playerNum;
int outPipe;

int main(int argc, char** argv){
    int err;
    pid_t myPid, parentPid;
    printf("\n\n");
    setupHandlers();
    
    //pids
    parentPid = getppid();
    myPid = getpid();
    srand(1000 * myPid);
    playerNum = myPid&1;
    outPipe = atoi(argv[1]);
    
  //player ready signal
    signalReady(parentPid);
    

    playingRPS();
    
    //terminate after
    printf("Player %d:      terminating\n", playerNum);
    return 0;
}//end main

//-------showHandler-------------
//signal handler for show signal
//--------------------------------
void showHandler(int sig){
    char choiceBuffer[2];
    int err;
    getChoice(choiceBuffer);
    err = write(outPipe, choiceBuffer, 2);
    if(err < 0){
        printf("Player.c: error in showHandler - %d\n", errno);
        exit(11);
    }//endif
    
}//endshowHandler

//------doneHandler-------------
//Signal handler for done signal
//--------------------------------
void doneHandler(int sig){
    printf("Player %d:      received done command\n", playerNum);
    playing = false;
}//end doneHandler


//--------setUpHandlers--------------
// Sets up signal handlers
//------------------------------------
void setupHandlers(){
    struct sigaction show_act;
    struct sigaction done_act;
    int err;
    sigset_t mask;
    sigemptyset(&mask); //Create mask, initialize to empty
    
    show_act.sa_handler = (void(*)(int))showHandler;
    show_act.sa_mask = mask;
    show_act.sa_flags = 0;
    err = sigaction(SIGUSR1, &show_act, NULL);
    if(err < 0){
        printf("player.c: Error %d on sigaction.\n", errno);
        exit(7);
    }//end if

    done_act.sa_handler = (void(*)(int))doneHandler;
    done_act.sa_mask = mask;
    done_act.sa_flags = 0;
    err = sigaction(SIGUSR2, &done_act, NULL);
    if(err < 0){
        printf("player.c: Error %d on sigaction.\n", errno);
        exit(8);
    }//endif
}//endsetupHandlers


//-------signalReady----------------
// Signals to the parents that the
// player is ready
//--------------------------------
void signalReady(pid_t parent){
    int err;
    sleep(playerNum); //Allow controller to reach Pause, ensure that not signaling
                      //At the same time.
    printf("Player %d       Sending Ready Command\n", playerNum);
    err = kill(parent, SIGUSR1);
    if(err < 0){
        printf("player.c: Error in sending Ready signal to parent: %d\n", errno);
        exit(6);
    }//endif
}//endsignalReady


//-----playingRPS--------------------
// Plays until the controller sends
// done signal
//------------------------------------
void playingRPS(){
    while(playing)
        pause(); //Wait for "Show" signal

}//endplayingRPS


//------getChoice--------------
// Rock Paper Scissor
//-----------------------------
void getChoice(char* buff){
    switch(rand() % 3){
        case 0:
            printf("Player %d: Selects   Rock\n", playerNum);
            sprintf(buff, "0");
            break;
        case 1:
            printf("Player %d: Selects   Paper\n", playerNum);
            sprintf(buff, "1");
            break;
        default:
            printf("Player %d: Selects   Scissors\n", playerNum);
            sprintf(buff, "2");
    }//endswitch
}//endgetChoice
