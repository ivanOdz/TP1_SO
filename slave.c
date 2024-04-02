// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define HASHING_ALGORITHM   "md5sum"

#define DEFAULT_BUFFER_SIZE 1024
#define HA_RESULT_SIZE      32
#define HA_BUFFER_SIZE      256

#define ORDINARY_ERROR      -1

#define READ_END            0
#define WRITE_END           1

#define TASK_ERROR_MESSAGE  "Error reading task"
#define POPEN_ERROR_MESSAGE "Error at popen"

int main(int argc, char * argv[]) {

    char * pathOfFileForHA;
    char algorithmResult[HA_RESULT_SIZE];
    char readBuffer[DEFAULT_BUFFER_SIZE];
    char writeBuffer[DEFAULT_BUFFER_SIZE];
    char commandBuffer[DEFAULT_BUFFER_SIZE];

    int status = EXIT_FAILURE;
    int pendingReadBufferBytes = 0;
    int writeBufferBytes = 0;
    int myPid = getpid();

    FILE * result;

    do {
        int readBytes = read(STDIN_FILENO, readBuffer, sizeof(readBuffer));

        if (readBytes == 0) {
            exit(EXIT_SUCCESS);
        }
        if (readBytes < 0) {
            perror(TASK_ERROR_MESSAGE);
            exit(ORDINARY_ERROR);
        }
        if (readBuffer[readBytes - 1] == '\n') {
            readBuffer[--pendingReadBufferBytes] = '\0';
        }
        size_t offset = 0;

        while (offset < readBytes) {
            pathOfFileForHA = readBuffer + offset;
            snprintf(commandBuffer, DEFAULT_BUFFER_SIZE, "%.*s %.*s", (int)sizeof(HASHING_ALGORITHM) - 1, HASHING_ALGORITHM, (int)strlen(pathOfFileForHA), pathOfFileForHA);
            result = popen(commandBuffer, "r");

            if (result == NULL) {
                perror(POPEN_ERROR_MESSAGE);
                exit(ORDINARY_ERROR);
            }
            fgets(algorithmResult, HA_RESULT_SIZE, result);
            writeBufferBytes = snprintf(writeBuffer, DEFAULT_BUFFER_SIZE, "%s - %.*s - %d\n", pathOfFileForHA, HA_RESULT_SIZE, algorithmResult, myPid);
            write(STDOUT_FILENO, writeBuffer, writeBufferBytes + 1);
            offset += strlen(pathOfFileForHA) + 1;
            pclose(result);
        }
    } while (1);

    return status;
}
