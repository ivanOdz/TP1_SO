// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

// Slave: Recibe los paths a los archivos que hay que procesar con el proceso "md5sum", luego enviar el hash generado al proceso app mediante pipe.
// Deberia esperar que el padre cierre el pípe, la lectura devuelve 0 y (tamb significa End of File). getline(3): si no se libera *line vamos a tener un memory leak

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define HASHING_ALGORITHM   "./md5sum"
#define NUMBER_ARGV_HA      2
#define HA_RESULT_BYTES     33

#define ORDINARY_ERROR      -1
#define FORK_CHILD          0
#define FORK_ERROR          ORDINARY_ERROR

#define READ_END            0
#define WRITE_END           1

#define BUFFER_SIZE         1024

void ordinaryErrorHandler(int status);

int main(int argc, char *argv[]) {

    int pid;
    int status;
    int pipeFd[2];
    char *argvForHA[NUMBER_ARGV_HA];
    char **envpForHA = NULL;
    char *pathOfFileForHA;
    char algorithmResult[HA_RESULT_BYTES];
    char readBuffer[BUFFER_SIZE];
    char writeBuffer[BUFFER_SIZE];
    int readBufferBytes = 0;
    int writeBufferBytes = 0;

    argvForHA[NUMBER_ARGV_HA-1] = NULL;

    do {

        if (readBufferBytes <= 1) {

            readBufferBytes = read(STDIN_FILENO, readBuffer, BUFFER_SIZE);

            if (readBufferBytes < 1) {

                status = EXIT_SUCCESS;
                break;
            }
        }

        do { // Buscar próximo Path (se consumen de forma LIFO)
            
            readBufferBytes--;

        } while (readBufferBytes && readBuffer[readBufferBytes] != '\0');

        if (pipe(pipeFd) == ORDINARY_ERROR) {

            ordinaryErrorHandler(status);
        }

        pid = fork();

        if (pid == FORK_CHILD) {
            
            close(pipeFd[READ_END]);

            if (dup2(pipeFd[WRITE_END], STDOUT_FILENO) == 0) {

                ordinaryErrorHandler(status);
            }

            close(pipeFd[WRITE_END]);

            pathOfFileForHA = readBuffer + readBufferBytes;
            argvForHA[NUMBER_ARGV_HA-2] = pathOfFileForHA;

            if (execve(HASHING_ALGORITHM, argvForHA, envpForHA) == 0) {

                ordinaryErrorHandler(status);
            }
        }
        else if (pid != FORK_ERROR) {

            close(pipeFd[WRITE_END]);

            if (read(pipeFd[READ_END], algorithmResult, sizeof(algorithmResult)) != sizeof(algorithmResult)) {

                ordinaryErrorHandler(status);
            }

            waitpid(pid, &status, 0);

            close(pipeFd[READ_END]);

            writeBufferBytes = sprintf(writeBuffer, "%*s_%d", (int)sizeof(algorithmResult)-1, algorithmResult, getpid());

            write(STDOUT_FILENO, writeBuffer, writeBufferBytes);
        }
        else {

            ordinaryErrorHandler(status);
        }

    } while (1);

    return status;
}

inline void ordinaryErrorHandler(int status) {

    exit(status);
}
