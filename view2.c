// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MAP_LENGTH            2000000 
#define SHM_ERROR_MESSAGE_1   "Unable to process shared memory handle"
#define SHM_ERROR_MESSAGE_2   "Error opening shared memory"
#define SHM_ERROR_MESSAGE_3   "Error mapping shared memory"
#define ORDINARY_ERROR        -1

int getLineLen(char * string);
char * openShm(char * handle);

int main(int argc, char * argv[]) {

    int fifo = open("/fifo", O_RDONLY);
    char buffer[100000];
    while(read(fifo, buffer, 100000) > 0){
        printf("%s", buffer);
    }
    return 0;
}

int getLineLen(char * string) {

    int len = 1;

    while (string[len] != '\n' && string[len] != 0) {
        len++;
    }
    return len;
}

char * openShm(char * handle) {

    int fd = shm_open(handle, O_RDWR, 0600);

    if (fd < 0) {
        perror(SHM_ERROR_MESSAGE_2);
        exit(ORDINARY_ERROR);
    }
    char * shmMap = mmap(NULL, MAP_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if ((long)shmMap < 0) {
        perror(SHM_ERROR_MESSAGE_3);
        exit(ORDINARY_ERROR);
    }
    close(fd);     
    return shmMap;
}