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

#define MAP_LENGTH 2000000 //2MB

int getLineLen(char* string);
char* openShm(char* handle);

int main(int argc, char* argv[]) {
    char* sharedMem;

    if (argc > 1) {
        sharedMem = openShm(argv[1]);
    } else {
        size_t bufferSize = 1024;
        char* sharedMemHandle = (char*) malloc(bufferSize * sizeof(char));
        if (sharedMemHandle == NULL) {
            perror("Unable to process shared memory handle");
            exit(1);
        }
        int readlen = getline(&sharedMemHandle, &bufferSize, stdin);
        sharedMemHandle[readlen-1] = 0;
        sharedMem = openShm(sharedMemHandle);
        free(sharedMemHandle);
    }

    sem_t* mutex = (sem_t*) sharedMem;
    char* resultBuffer = sharedMem + sizeof(sem_t);
    size_t offset = 0;

    do {
        sem_wait(mutex);
        int lineLength = getLineLen(resultBuffer + offset);
        fwrite(resultBuffer + offset, lineLength, 1, stdout);
        offset += lineLength;
    } while (resultBuffer[offset] != 0);

    munmap(sharedMem, MAP_LENGTH);
    return 0;
}

int getLineLen(char* string) {
    int len = 1;
    while (string[len] != '\n' && string[len] != 0) len++;
    return len;
}

char* openShm(char* handle) {
    printf("%s\n", handle);
    int fd = shm_open(handle, O_RDWR, 0600);
    if (fd == -1) {
        perror("Error opening shared memory");
        exit(1);
    }
    char* shmMap = mmap(NULL, MAP_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ((long)shmMap == -1) {
        perror("Error mapping shared memory");
        exit(1);
    }
    close(fd);      //fd can be closed without affecting shm mapping
    return shmMap;
}