// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <sys/mman.h>


#define SHMNAME "/app_shm_memory"
#define SHMSIZE 2000000
#define SLAVESQTY 5

#define READ_END 0
#define WRITE_END 1


#define INSSUFICIENTARGS_ERR -1
#define SHMOPEN_ERR -2
#define FTRUNCATE_ERR -3
#define MMAP_ERR -4
#define PIPE_ERR -5
#define FORK_ERR -6
#define EXCECVE_ERR -7

int main(int argc, char * argv[]) {
    //Minimo tiene que tener 2 argumentos. El primero es el nombre del programa y el segundo es 1 archivo.
    if (argc < 2) {
        // Manejo de errores para cantidad de argumentos insuficientes
        perror("Inssuficient arguments.");
        exit(INSSUFICIENTARGS_ERR);
    }
    // Si estoy en este punto, tengo al menos 1 archivo para procesar, contiuo.

    // Creo la sharedMemory, y muestro su nombre por la stdout. Hago un wait por 2 segundos para esperar a que se
    // conecte un proceso vista. Caso contrario reanudo el programa sin inconveientes.
    int shmMemFd = shm_open(SHMNAME,  O_CREAT | O_RDWR | O_EXCL, 0600);
    if(shmMemFd < 0) {
        perror("shm_open");
        exit(SHMOPEN_ERR);
    }
    if (ftruncate(shmMemFd, SHMSIZE) < 0) {
        exit(FTRUNCATE_ERR);
    }
    char * shmAddr;
    shmAddr = mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmMemFd, 0);
    if (shmAddr == MAP_FAILED) {
        perror("mmap");
        exit(MMAP_ERR);
    }
    close(shmMemFd);

    // Colocar mutex en primera posicion de la shm

    printf("%s\n",SHMNAME);
    sleep(2);

    // Tengo que crear 2 PIPES por cada SLAVE, uno de escritura y otro de lectura.
    // Tengo que crear los procesos SLAVES, mediante fork y execve, manejando los posibles errores en ambos casos.
    int anspipefd[2];    //var temporal de fds del pipe de donde lee la respuesta app
    int taskpipefd[2];   //var temporal de fds del pipe donde app pasa argumentos al esclavo

    int writefds[SLAVESQTY];    //var que recolecta todos los fds de escritura (app) de los pipes
    int readfds[SLAVESQTY];   //var que recolecta todos los fds de lectura (app) de los pipes

    pid_t cpid;
    for(int slave = 0; slave < SLAVESQTY; slave++) {
        //Creo los 2 pipes
        if(pipe(taskpipefd) < 0) {
            perror("pipe");
            exit(PIPE_ERR);
        }
        if(pipe(anspipefd) < 0) {
            perror("pipe");
            exit(PIPE_ERR);
        }
        cpid = fork();
        if (cpid < 0) {
            perror("fork");
            exit(FORK_ERR);
        }
        if(cpid == 0) {
            // Dentro del hijo, cierro fds que no uso
            close(READ_END);
            close(WRITE_END);
            close(anspipefd[0]);    //donde escribo, no me interesa leer
            close(taskpipefd[1]);   //donde leo, no me interesa escribir

            // Acomodo los fds para que tomen el menor valor posible
            dup(taskpipefd[0]);     // Tengo que duplicar ese primero ya que desde aca leo -> fd:0
            close(taskpipefd[0]);   // Luego borro el original
            dup(anspipefd[1]);      // Duplico para que el write me quede en fd:1
            close(anspipefd[1]);    // Borro el original

            // Borro todas los fd de otros pipes.
            for(int i = 0; i < slave; i++) {
                close(writefds[i]);
                close(readfds[i]);
            }

            //Llamo a execve.
            char * args1[] = {"./slave", NULL};
            int err1 = execve(args1[0], args1, NULL);

            if(err1 < 0){
                perror("execve");
                exit(EXCECVE_ERR);
            }
        }
        // Cierro los fds que el padre no utiliza
        close(taskpipefd[0]);
        close(anspipefd[1]);

        // Acomodo los fds para que tomen el menor valor posible y los guardo
        //writefds[slave] = dup(taskpipefd[1]);
        //close(taskpipefd[1]);

        readfds[slave] = dup(anspipefd[0]);
        close(anspipefd[0]);
        writefds[slave] = taskpipefd[1];
    }

    // A medida que los SLAVES escriben en el buffer de lectura, el cual app se queda "escuchando" mediante select,
    // app hace un read y escribe eso mismo en la shared memory (mediante getLine, liberar las lineas)
    // luego le da al hijo que retorno algo, otro path a otro archivo. Esto se realiza mediante tenga hijos que no
    // hayan retornado. Una vez que retornan todos los hijos, finaliza el procesamiento.

    // Al final de programa, copio todo lo de la sharedMemory en un archivo resultado.txt.

    shm_unlink(SHMNAME);
    exit(0);
}