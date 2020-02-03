#include <stdio.h> 
#include <signal.h> 
#include <unistd.h>
#include <stdlib.h>

// user-defined signal handler for alarm. 
void alarm_handler(int signo){
    if (signo == SIGALRM){ 
        printf("Alarm\n");
    }
}

// user-defined signal handler for int. 
void int_handler(int signo){
    if (signo == SIGINT){ 
        printf("CTRL+C pressed!\n");
    }
}

// user-defined signal handler for stop. 
void tstp_handler(int signo){
    if (signo == SIGTSTP){ 
        printf("CTRL+Z pressed!\n");
        exit(0); //exit program immediately 
    }
}

int main(void){
    // register signal handlers
    if (signal(SIGALRM , alarm_handler) == SIG_ERR){
        printf("failed to register alarm handler.");
        exit(1); 
    }

    if (signal(SIGINT , int_handler) == SIG_ERR){
        printf("failed to register int handler.");
        exit(1); 
    }

    if (signal(SIGTSTP , tstp_handler) == SIG_ERR){
        printf("failed to register tstp handler.");
        exit(1); 
    }

    while(1){ 
        sleep(2);
        raise(SIGALRM); 
    } // raise alarm signal every 2 seconds
}