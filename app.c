// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#define SLAVEPROCESS            "./slave"
#define RESULTFILE              "results.txt"
#define SHMNAME                 "/app_shm"
#define SHMSIZE                 2000000
#define SLAVESQTY               10
#define INITIAL_TASKS           3
#define BUFFER_SIZE             1024
#define SLAVEDEAD               -1
#define ERROR                   -1

#define READ_END                0
#define WRITE_END               1

#define SHM                     1
#define SEM                     2
#define SLAVES                  4
#define WORKING                 8

#define EARGS                   "Insufficient arguments. Please provide at least one file"
#define EREADSLAVE              "Error reading result from slave"
#define EBUFFULL                "Result Buffer full"
#define ESHM                    "Error creating shared memory"
#define EPIPES                  "Error creating slave pipes"
#define ERUNSLAVE               "Couldn't execute slave"
#define ERESULTFILE             "Error writing results to file"


size_t shmWrite(char * shmBuffer, char * str, sem_t * mutex);
char * createSHM(char * name, size_t size);
void sendSlaveTask(char * path, int fd);
void createSlaves(int * readEndpoints, int * writeEndpoints, int amount);
void killSlave(int * readfds, int * writefds, int slave);
void errorAbort(char * message);
int Dup(int oldfd);
void dumpToFile(char * source);
void Exit(int returnValue);


char * shmAddr = NULL;


int main(int argc, char * argv[]) {

    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 2) {
        errorAbort(EARGS);
    }
    
    shmAddr = createSHM(SHMNAME, SHMSIZE);
    char * shmBuffer = shmAddr + sizeof(sem_t);
    sem_t * mutex = (sem_t *) shmAddr;
    size_t offset = 0;
    if (sem_init(mutex, 1, 0)){
        errorAbort(ESHM);
    }   
    
    printf("%s\n", SHMNAME);
    sleep(2); 

    int writefds[SLAVESQTY] = {[0 ... SLAVESQTY-1] = SLAVEDEAD};
    int readfds[SLAVESQTY] = {[0 ... SLAVESQTY-1] = SLAVEDEAD};
    int slaveTasks[SLAVESQTY] = {0};

    int slaves = SLAVESQTY;

    if (argc < SLAVESQTY) {
        slaves = argc - 1;
    }
    createSlaves(readfds, writefds, slaves);
    int filesAssigned = 0;
    int initialAssign = INITIAL_TASKS;

    if (argc - 1 < slaves * INITIAL_TASKS) {
        initialAssign = (argc - 1) / slaves;
    }
    for (int i = 0; i < slaves; i++) {

        for (int j = 0; j < initialAssign; j++) {
            sendSlaveTask(argv[1 + i * initialAssign + j], writefds[i]);
            slaveTasks[i]++;
        }
        filesAssigned += initialAssign;
    }
    fd_set set;
    int filesProcessed = 0;
    int maxFD = 0;
    for (int i = 0; i < slaves; i++) {
        if (readfds[i] > maxFD) {
            maxFD = readfds[i];
        }
    }
    char resultBuffer[BUFFER_SIZE];

    while (slaves) {
        FD_ZERO(&set);
        for (int i = 0; i < SLAVESQTY; i++) {
            if (readfds[i] != SLAVEDEAD) {
                FD_SET(readfds[i], &set);
            }
        }
        if (select(maxFD + 1, &set, NULL, NULL, NULL) == ERROR) {
            errorAbort(EREADSLAVE);
        }

        for (int i = 0; i < SLAVESQTY; i++) {
            if (readfds[i] != SLAVEDEAD && FD_ISSET(readfds[i], &set)) {
                FD_CLR(readfds[i], &set);
                ssize_t bytesread = read(readfds[i], resultBuffer, BUFFER_SIZE);

                if (bytesread == ERROR) {
                    errorAbort(EREADSLAVE);
                }
                if (offset + bytesread > SHMSIZE) {
                    errorAbort(EBUFFULL);
                }

                size_t written = 0;
                while (written < (size_t)bytesread) {
                    int temp = shmWrite(shmBuffer + offset, resultBuffer + written, mutex);
                    written += temp + 1;
                    filesProcessed++;
                    offset += temp;
                    slaveTasks[i]--;
                }

                if (filesAssigned < argc - 1) {
                    if (!slaveTasks[i]) {
                        sendSlaveTask(argv[++filesAssigned], writefds[i]);
                        slaveTasks[i]++;
                    }
                } else {
                    if (!slaveTasks[i]){
                        killSlave(readfds, writefds, i);
                        slaves--;
                    }
                }
                i = 0;
            }
        }
    }
    Exit(EXIT_SUCCESS);
}

size_t shmWrite(char * shmBuffer, char * str, sem_t * mutex) {
    size_t len = strlen(str);
    memcpy(shmBuffer, str, len);
    sem_post(mutex);
    return len;
}

char * createSHM(char * shmName, size_t size) {
    shm_unlink(SHMNAME);
    int shmMemFd = shm_open(shmName,  O_CREAT | O_RDWR, 0600);

    if (shmMemFd == ERROR) {
        errorAbort(ESHM);
    }
    if (ftruncate(shmMemFd, size) == ERROR) {
        errorAbort(ESHM);
    }

    char * shmAddr;
    shmAddr = mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmMemFd, 0);

    if (shmAddr == MAP_FAILED) {
        errorAbort(ESHM);
    }

    close(shmMemFd);
    return shmAddr;
}

void sendSlaveTask(char * path, int fd) {
    write(fd, path, strlen(path) + 1);
}

void createSlaves(int * readfds, int * writefds, int amount) {
    int anspipefd[2];
    int taskpipefd[2];
    pid_t cpid;

    for (int slave = 0; slave < amount; slave++) {
        if (pipe(taskpipefd) == ERROR) {
            errorAbort(EPIPES);
        }
        if  (pipe(anspipefd) == ERROR) {
            errorAbort(EPIPES);
        }

        cpid = fork();
        if (cpid == ERROR) {
            errorAbort(ERUNSLAVE);
        }
        if (cpid == 0) {
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(anspipefd[READ_END]);
            close(taskpipefd[WRITE_END]);

            Dup(taskpipefd[READ_END]);
            close(taskpipefd[READ_END]);
            Dup(anspipefd[WRITE_END]);
            close(anspipefd[WRITE_END]);

            for (int i = 0; i < slave; i++) {
                close(writefds[i]);
                close(readfds[i]);
            }

            char * args1[] = {SLAVEPROCESS, NULL};
            execve(args1[0], args1, NULL);
            errorAbort(ERUNSLAVE);
        }
        close(taskpipefd[READ_END]);
        close(anspipefd[WRITE_END]);

        readfds[slave] = Dup(anspipefd[READ_END]);
        close(anspipefd[READ_END]);
        writefds[slave] = taskpipefd[WRITE_END];
    }
}

void killSlave(int * readfds, int * writefds, int slave) {
    close(readfds[slave]);
    close(writefds[slave]);
    readfds[slave] = -1;
    writefds[slave] = -1;
}

int Dup(int oldfd) {
    int newfd = dup(oldfd);
    if (newfd == ERROR) {
        errorAbort(EPIPES);
    }
    return newfd;
}

void errorAbort(char * message) {
    if (!errno) {
        fprintf(stderr, "%s\n", message);
    } else {
        perror(message);
    }
    Exit(-1);
}

void Exit(int returnValue){
    if (shmAddr != NULL) {
        sem_post((sem_t *)shmAddr);
        dumpToFile(shmAddr + sizeof(sem_t));
        sem_destroy((sem_t *)shmAddr);
        shm_unlink(SHMNAME);
    }
    exit(returnValue);
}

void dumpToFile(char * source){
    int resultfd = creat(RESULTFILE, 0600);
    if (resultfd == ERROR) {
        perror(ERESULTFILE);
        return;
    } 
    if (write(resultfd, source, strlen(source)) == -1) {
        perror(ERESULTFILE);
    }
    close(resultfd);
    return;
}