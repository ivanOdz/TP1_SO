// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// Slave: Recibe los paths a los archivos que hay que procesar con el proceso "md5sum", luego enviar el hash generado al proceso app mediante pipe.
// Deberia esperar que el padre cierre el pípe, la lectura devuelve 0 y (tamb significa End of File). getline(3): si no se libera *line vamos a tener un memory leak

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define HASHING_ALGORITHM   "md5sum"
#define NUMBER_ARGV_HA      4
#define HA_RESULT_SIZE      32
#define HA_BUFFER_SIZE      256

#define ORDINARY_ERROR      -1
#define FORK_CHILD          0
#define FORK_ERROR          ORDINARY_ERROR

#define READ_END            0
#define WRITE_END           1

#define DEFAULT_BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    int status = EXIT_FAILURE;

    char *pathOfFileForHA;
    char algorithmResult[HA_RESULT_SIZE];
    char readBuffer[DEFAULT_BUFFER_SIZE];
    char writeBuffer[DEFAULT_BUFFER_SIZE];
    int pendingReadBufferBytes = 0;
    int writeBufferBytes = 0;
    int myPid = getpid();
    int firstRead = 1;
    FILE * result;

    do {

        if (pendingReadBufferBytes == 0) {

            pendingReadBufferBytes = read(STDIN_FILENO, readBuffer, sizeof(readBuffer));
    
            if (pendingReadBufferBytes < 1) {

                status = EXIT_SUCCESS;
                break;
            }

            if (readBuffer[pendingReadBufferBytes-1] == '\n') {

                pendingReadBufferBytes--;
            }

            readBuffer[pendingReadBufferBytes] = '\0';

            if (!firstRead) {

                pathOfFileForHA = readBuffer;
                pendingReadBufferBytes = 0;
            }
        }

        if (firstRead) {

            while (pendingReadBufferBytes && readBuffer[pendingReadBufferBytes-1] != '\0') {

                pendingReadBufferBytes--;
            }
            if (pendingReadBufferBytes == 0) {
                
                firstRead = 0;
            }

            pathOfFileForHA = readBuffer + pendingReadBufferBytes;
        }

        char command[DEFAULT_BUFFER_SIZE];
        snprintf(command, DEFAULT_BUFFER_SIZE, "%.*s %.*s", (int)strlen(HASHING_ALGORITHM), HASHING_ALGORITHM, (int)strlen(pathOfFileForHA), pathOfFileForHA);
        result = popen(command, "r");
    
        if (result == NULL) {
            perror("popen");
            exit(ORDINARY_ERROR);
        }

        fgets(algorithmResult, HA_RESULT_SIZE, result);
        writeBufferBytes = snprintf(writeBuffer, DEFAULT_BUFFER_SIZE, "%s - %.*s - %d\n", pathOfFileForHA, HA_RESULT_SIZE, algorithmResult, myPid);
        write(STDOUT_FILENO, writeBuffer, writeBufferBytes + 1);
        
    } while (1);


    return status;
}
