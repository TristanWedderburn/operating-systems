#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define KEY (2020)

int getFileSize(int file){
    struct stat fileStat;

    if(fstat(file, &fileStat) < 0){
        printf("Could not read size from file descriptor\n");
        return 0;
    }  
        
    return fileStat.st_size;
}

char * allocateMemory(char* filename){
    int file = 0, fileSize = 0;
    char * memoryMap; //pointer to mapped memory

    if ((file = open(filename, O_RDWR)) < -1){
        printf("Could not open file\n");
        return "\0";
    }

    fileSize = getFileSize(file); //ensure that the allocated memory is greater than the physical file size

    memoryMap = mmap(NULL, fileSize, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, file, 0); //allocate memory for file descriptor

    if (memoryMap == MAP_FAILED) {
        printf("Could not mmap file");
        return "\0";
    }

    return memoryMap;
}

void syncResourceAllocation(char * memoryMap){ //update res.txt file
    int sync_result = msync(memoryMap, strlen(memoryMap), MS_SYNC | MS_INVALIDATE ); //allocate memory for file descriptor

    if (sync_result < 0) {
        perror("Could not msync file");
    }
}

void consumeResource(char * memoryMap, int resource_type, int resources_left){ //write new value into memory map
    if(resources_left < 0){
        printf("You requested too many resources. Could not process request\n");
        return;
    }

    char value = resources_left+'0'; //convert int to char;
    memoryMap[resource_type+2] = value;
    syncResourceAllocation(memoryMap); //sync memory map with physical memory
}

void P(struct sembuf * ops, int sem){
    if(semop(sem, ops, 1) < 0){
       printf("p-operation failed.");
    }
}

void V(struct sembuf * ops, int sem){
    if(semop(sem, ops, 1) < 0) {
       printf("v-operation failed.");
    }
}

int initializeSemaphore(struct sembuf * p_ops, struct sembuf * v_ops){
    int sem = semget(KEY, 1, 0666 | IPC_CREAT);

    v_ops[0].sem_num = 0;
    v_ops[0].sem_op = 1;  
    v_ops[0].sem_flg = 0;

    p_ops[0].sem_num = 0;
    p_ops[0].sem_op = -1;  
    p_ops[0].sem_flg = 0;

    if(sem < 0){
        printf("Semget failed.\n");
        return -1;
    }

    if(semctl(sem, 0, SETVAL, 1) < 0) {
        printf("Cannot set semaphore value.\n");
    }

    return sem;
}

int main ( void ) {
    int sem, resource_type = 0, resources_available = 0, resources_needed = 0, resources_left = 0;
    char * memoryMap;
    struct sembuf p_ops[1];
    struct sembuf v_ops[1];

    sem = initializeSemaphore(p_ops, v_ops);
    
    P(p_ops, sem);
    memoryMap = allocateMemory("res.txt");
    V(v_ops, sem);

    while(true){
        P(p_ops, sem);
        for(int i = 0; i < strlen(memoryMap); i+=4){ //each line is 4 characters long
            resource_type = memoryMap[i] - '0';
            resources_available = memoryMap[i + 2] - '0';
            printf("Resources available for resource type %d: %d\n", resource_type, resources_available);
            printf("Please enter amount of resources needed: \n");
            scanf("%d", &resources_needed);

            if(resources_needed < 0){
                printf("Resources needed must be >= 0");
                continue;
            }
               
            resources_left = resources_available - resources_needed; //subtract from resources available
            consumeResource(memoryMap, i, resources_left);
        }
        V(v_ops, sem);
    }

    return 0;
}


