#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>

int main ( void ) {
    int toparent[2] , tochild[2], parentbytes, childbytes ;
    pid_t childpid ;

    char readbuffer [80];
    char writebuffer [80]; //buffer variable to send across pipe

    int childInput; //1 byte input from keyboard
    int parentInput;

    int sum = 0; //sum digits from keyboard
    bool done = false; //flag for finished process

    //create pipes
    if (pipe (toparent) == -1 || pipe(tochild) == -1) {
        printf("error creating pipes");
        exit(1);
    }

    // fork child process
    if ((childpid = fork()) == -1) {
        perror("fork");
        exit(0);
    }

    if(childpid == 0) {
        close(toparent[0]);
        close(tochild[1]);

        while(1){
            if(done == 1) {
                childbytes = read(tochild[0], &readbuffer, sizeof(readbuffer));

                if (childbytes > 0) {
                    printf("Sum: %s\n", readbuffer);
                    exit(0);
                }
                else{
                    break;
                }
            }

            printf("Please enter a character: \n");
            scanf("%d", &childInput); // get 1 char from the input

            if(childInput == -1){
                done = true;
            }

            snprintf(writebuffer, sizeof(childInput), "%d", childInput); //convert to string to send across pipe
            write(toparent[1], &writebuffer, sizeof(writebuffer)); //send value to parent from child using pipe
        }

        close(toparent[1]);
        close(tochild[0]);
    }
    else {
        close(toparent[1]);
        close(tochild[0]);

        while(1) {
            parentbytes = read(toparent[0], &readbuffer, sizeof(readbuffer));

            if (parentbytes > 0) {
                sscanf(readbuffer, "%d", &parentInput);

                if(parentInput == -1) {
                    snprintf(writebuffer, sizeof(sum), "%d", sum); //convert to string to send across pipe

                    write(tochild[1], &writebuffer, sizeof(writebuffer)); //send value to parent from child using pipe
                    exit(0);
                }
                else {
                    sum+=parentInput;
                }
            }
            else {
                break;
            }
        }

        close(toparent[0]);
        close(tochild[1]);
    }
}
