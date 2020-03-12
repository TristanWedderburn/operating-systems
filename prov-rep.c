#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
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
        perror("Could not mmap file");
        return "\0";
    }

    return memoryMap;
}

void syncResourceAllocation(char * memoryMap){ //update res.txt file
    int sync_result = 0;
    
    sync_result = msync(memoryMap, strlen(memoryMap), MS_SYNC | MS_INVALIDATE ); //allocate memory for file descriptor

    if (sync_result < 0) {
        perror("Could not msync file");
    }
}

void produceResource(char * memoryMap, int resource_type, int resources_available){ //write new value into memory map
    if(resources_available < 0 || resources_available >= 10){
        printf("Invalid number of resources to add. Could not process request\n");
        return;
    }

    char value = resources_available+'0'; //convert int to char
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
    int sem, add_resources = 0, resource_type = 0, resources_available = 0, units_to_add = 0;
    pid_t childpid ;
    char * memoryMap;
    struct sembuf p_ops[1];
    struct sembuf v_ops[1];

    sem = initializeSemaphore(p_ops, v_ops);
    
    P(p_ops, sem);
    memoryMap = allocateMemory("res.txt");
    V(v_ops, sem);

    // fork child process
    if ((childpid = fork()) == -1) {
        perror("fork");
        exit(0);
    }

    if(childpid == 0) {
        while(1){
            sleep(10); //delay for 10s

            P(p_ops, sem);
            int pagesize = getpagesize();
            int length = strlen(memoryMap);
            int vec_length = (length + pagesize -1)/ pagesize;
            char status[vec_length];
            char *vec = status;
            int page_status = 0;

            printf("System page size: %d\n", pagesize);
            printf("Current Resource Allocation:\n%s\n", memoryMap);

            if(mincore(memoryMap, length, vec) < 0){
                printf("mincore failed\n");
            } else{
                for(int i =0; i< vec_length;i++){ //loop through vec to check page status
                    page_status = (int)vec[i] & 1; //check first bit of each byte for page status
                    if(page_status == 1){ 
                        printf("Page %d is residing in memory\n", i);
                        continue;
                    }
                    printf("Page %d is not residing in memory\n", i);
                }
            }
            V(v_ops, sem);
        }
    }
    else {
        while(1) {
            printf("Add new resources? {1: yes, 0: no}: \n");
            scanf("%d", &add_resources);
            
            if(add_resources == 1){
                P(p_ops, sem);
                printf("Please enter in the resource type to add to {0,1,2}: \n");
                scanf("%d", &resource_type);
                printf("Please enter number of resource units to add: \n");
                scanf("%d", &units_to_add); 

                if(units_to_add < 0){
                    printf("Units to add must be >= 0\n");
                    continue;
                }

                resources_available = (memoryMap[4*resource_type+2] - '0') + units_to_add;
                produceResource(memoryMap, 4*resource_type, resources_available);
                V(v_ops, sem);
            }
        }
    }
}
