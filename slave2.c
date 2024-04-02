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
    setvbuf(stdout, NULL, _IONBF, 0);
    char buff[1024];
    int red = 0;
    while ((red = read(0, buff, 1024)) != 0){
        buff[red - 1] = '\n';
        buff[red] = 0;
        write(1, buff, red + 1);
    }
    return 0;
}
