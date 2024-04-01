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
    char *envpForHA = NULL;
    char *pathOfFileForHA;
    char algorithmResult[HA_RESULT_BYTES];
    char buffer[BUFFER_SIZE];
    int bufferBytes = 0;

    argvForHA[NUMBER_ARGV_HA-1] = NULL;

    do {

        if (bufferBytes <= 1) {

            bufferBytes = read(STDIN_FILENO, buffer, BUFFER_SIZE);

            if (bufferBytes < 1) {

                status = EXIT_SUCCESS;
                break;
            }
        }

        do { // Buscar próximo Path (se consumen de forma LIFO)
            
            bufferBytes--;

        } while (bufferBytes && buffer[bufferBytes] != ' ');

        if (pipe(pipeFd) == ORDINARY_ERROR) {

            ordinaryErrorHandler(status);
        }

        pid = fork();

        if (pid == FORK_CHILD) {
            
            close(pipeFd[READ_END]);
            dup2(pipeFd[WRITE_END], STDOUT_FILENO); 
            close(pipeFd[WRITE_END]);

            pathOfFileForHA = buffer + bufferBytes;
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

            write(algorithmResult, sizeof(algorithmResult), STDOUT_FILENO);
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
