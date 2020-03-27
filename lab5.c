// params: string --> file or directory name
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define MAX   80 

/*
    Function: print_level

    prints a string that indents the file attributes to show directory levels

    level: directory depth from directory where program was run

    returns: void
*/
void print_level(int level){
    printf("|");
    for(int i=0; i< level; i+=1){
        printf("____");
    }
}

/*
    Function: get_current_working_directory

    Gets the full path of the current directory

    returns: string of path
*/
char* get_current_working_directory(){
    long size;
    char *buf;
    char *ptr;

    size = pathconf(".", _PC_PATH_MAX);

    if ((buf = (char *)malloc((size_t)size)) != NULL){
        ptr = getcwd(buf, (size_t)size);
    }

    return ptr;
};

/*
    Function: get_directory_stream

    Gets pointer to directory stream to traverse

    directory_name: name of directory to traverse

    returns: pointer to directorys stream, if valid name, else NULL
*/
DIR* get_directory_stream(char* directory_name){
    DIR* directory = opendir(directory_name);

    if(directory == NULL){
        perror("open directory\n");
    }
    return directory;
};

/*
    Function: read_file

    prints attributes of given file

    directory_name: directory of file to print

    file_name: name of file to print

    level: directory deoth of file

    returns: void
*/
void read_file(char* directory_name, char* file_name, int level){
    struct stat sb;
    char full_file_name[MAX];

    //get full file path of file to read
    strcpy(full_file_name, directory_name);
    strcat(strcat(full_file_name,"/"), file_name);

    if(stat(full_file_name, &sb)==-1){
        perror("stat\n");
    }

    print_level(level);
    printf(
        "mode: %o, num of links: %hu, owner's name: %u, group name: %u, size (bytes): %lld, size (blocks): %d, last modified: %ld, name: %s\n", 
        sb.st_mode, sb.st_nlink, sb.st_uid, sb.st_gid, sb.st_size, sb.st_blksize, sb.st_mtime, file_name
    );
    return;
}

/*
    Function: get_path_mode

    Gets the mode of the inode at the input path

    path_name: path of inode to get mode of

    returns: mode of inode as mode_t
*/
mode_t get_path_mode(char* path_name){ //checks if the supplied argument is a file
    struct stat path_stat;

    if(stat(path_name, &path_stat) == -1){
        perror("stat\n");
    }
    return path_stat.st_mode;
}

/*
    Function: _traverse_directory

    recursive helper function to read the next inode in the directory stream

    directory: pointer to directory stream

    returns: struct containing information about inode
*/
struct dirent * _traverse_directory(DIR* directory){
    if(directory == NULL){ //end of directory stream
        return NULL;
    }

    errno = 0; //set errno before readdir to check end of stream or error
    struct dirent * res = readdir(directory);

    if(res == NULL && errno != 0){ //check if error reading directory
        perror("readdir");
    };
    return res;
}

/*
    Function: traverse_directory

    traverses directories and detemines when to read file or continuing traversing recursively

    directory_name: name of directory to traverse

    level: directory depth from directory where program was run

    returns: void
*/
void traverse_directory(char* directory_name, int level){
    DIR* directory = get_directory_stream(directory_name);
    struct dirent * inode = _traverse_directory(directory);
    char next_directory_name[MAX];

    while(inode != NULL){
        char * inode_name = inode->d_name;
        __uint8_t inode_type = inode->d_type;

        if(inode_type && inode_type == 8){
            read_file(directory_name, inode_name, level);
        } else if(inode_type && inode_type == 4 && (strcmp(inode_name, ".") && strcmp(inode_name, ".."))){ //inode is a directory but not . or ..
            strcpy(next_directory_name, directory_name);
            strcat(strcat(next_directory_name,"/"), inode_name);
            traverse_directory(next_directory_name, level+1); //recursive call to function for new directory
        }
        inode = _traverse_directory(directory);
    };
        
    if(closedir(directory) == -1){ //close directory stream after all inodes have been processed
        perror("close directory\n");
    };
    return;
}

/*
    Function: main

    high level logic that determines how to process input command line argument (file, directory or default to current directory)

    argc: number of command line arguments

    argv: array of command line arguments

    returns: returns 0 in succes or 1 on fail to process input
*/

int main(int argc, char *argv[]){
    printf(".\n");
    if( argc >= 2 ){ //command line argument supplied
        mode_t path_mode = get_path_mode(argv[1]);
        printf("mode: %hu\n", path_mode);

        if (S_ISREG(path_mode)){ //path is a file 
            read_file(get_current_working_directory(), argv[1], 1);
        } else if(S_ISDIR(path_mode)) { //path is a directory
            traverse_directory(argv[1], 1);
        } else{
            perror("input argument invalid\n");
            return 1;
        }
    } else { // no command line argument supplied
        traverse_directory(get_current_working_directory(), 1);
    }
    return 0;
}