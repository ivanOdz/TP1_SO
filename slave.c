// Slave: Recibe los paths a los archivos que hay que procesar con el proceso "md5sum", luego enviar el hash generado al proceso app mediante pipe.
// Deberia esperar que el padre cierre el pípe, la lectura devuelve 0 y (tamb significa End of File). getline(3): si no se libera *line vamos a tener un memory leak

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define HASHING_ALGORITHM   "./md5sum"
#define NUMBER_ARGV_HA      2
#define HA_RESULT_BYTES     33

#define ORDINARY_ERROR      -1
#define FORK_CHILD          0
#define FORK_ERROR          ORDINARY_ERROR

#define READ_END            0
#define WRITE_END           1

void ordinaryErrorHandler(int status);

int main(int argc, char *argv[]) {

    int pid;
    int status;
    int pipeFd[2];
    char *argvForHA[NUMBER_ARGV_HA];
    char *envpForHA = NULL;
    char *pathOfFileForHA;
    char algorithmResult[HA_RESULT_BYTES];

    argvForHA[NUMBER_ARGV_HA-1] = NULL;

    do {

        // read(pathOfFileForHA, lineLen, ?);
        if (pipe(pipeFd) == ORDINARY_ERROR) {

            ordinaryErrorHandler(status);
        }

        pid = fork();

        if (pid == FORK_CHILD) {
            
            // close(?);
            close(pipeFd[READ_END]);
            dup2(pipeFd[WRITE_END], STDOUT_FILENO);
            close(pipeFd[WRITE_END]);

            pathOfFileForHA = "?";
            argvForHA[NUMBER_ARGV_HA-2] = pathOfFileForHA;

            if (execve(HASHING_ALGORITHM, argvForHA, envpForHA) == 0) {

                ordinaryErrorHandler(status);
            }
        }
        else if (pid != FORK_ERROR) {

            close(pipeFd[WRITE_END]);
            waitpid(pid, &status, 0);

            if (read(pipeFd[READ_END], algorithmResult, sizeof(algorithmResult)) != sizeof(algorithmResult)) {
                ordinaryErrorHandler(status);
            }
            close(pipeFd[READ_END]);
            //write(algorithmResult, sizeof(algorithmResult), ?);
        }
        else {

            ordinaryErrorHandler(status);
        }

    } while (status != ORDINARY_ERROR); // && Tenga archivos por procesar

    status = 1;
    return status;
}

inline void ordinaryErrorHandler(int status) {

    exit(status);
}