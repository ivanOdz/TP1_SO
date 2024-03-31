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


#define INSSUFICIENTARGSERR -1
#define SHMOPENERR -2
#define FTRUNCATEERR -3
#define MMAPERR -4

int main(int argc, char * argv[]) {
    //Minimo tiene que tener 2 argumentos. El primero es el nombre del programa y el segundo es 1 archivo.
    if (argc < 2) {
        // Manejo de errores para cantidad de argumentos insuficientes
        perror("Inssuficient arguments.");
        exit(INSSUFICIENTARGSERR);
    }
    // Si estoy en este punto, tengo al menos 1 archivo para procesar, contiuo.

    // Creo la sharedMemory, y muestro su nombre por la stdout. Hago un wait por 2 segundos para esperar a que se
    // conecte un proceso vista. Caso contrario reanudo el programa sin inconveientes.
    int shmMemFd = shm_open(SHMNAME,  O_CREAT | O_RDWR | O_EXCL, 0600);
    if(shmMemFd < 0) {
        perror("Shared memory open error");
        exit(SHMOPENERR);
    }
    char shmbuff [SHMSIZE];
    if (ftruncate(shmMemFd, SHMSIZE) < 0) {
        exit(FTRUNCATEERR);
    }
    char * shmAddr;
    shmAddr = mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmMemFd, 0);
    if (shmAddr == MAP_FAILED) {
        exit(MMAPERR);
    }
    close(shmMemFd);

    printf("%s\n",SHMNAME);
    sleep(2);

    // Tengo que crear 2 PIPES por cada SLAVE, uno de escritura y otro de lectura.


    // Tengo que crear los procesos SLAVES, mediante fork y execve, manejando los posibles errores en ambos casos.



    // A medida que los SLAVES escriben en el buffer de lectura, el cual app se queda "escuchando" mediante select,
    // app hace un read y escribe eso mismo en la shared memory (mediante getLine, liberar las lineas)
    // luego le da al hijo que retorno algo, otro path a otro archivo. Esto se realiza mediante tenga hijos que no
    // hayan retornado. Una vez que retornan todos los hijos, finaliza el procesamiento.


    // Al final de programa, copio todo lo de la sharedMemory en un archivo resultado.txt.

    shm_unlink(SHMNAME);
    exit(0);
}