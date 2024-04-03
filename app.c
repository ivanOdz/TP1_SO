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


#define SHMNAME               "/app_shm"
#define SHMSIZE               2000000
#define SLAVESQTY             5
#define INITIAL_TASKS         3
#define BUFFER_SIZE           1024


#define READ_END              0
#define WRITE_END             1

#define ORDINARY_ERROR       -1
#define INSUFICIENT_ARGS_ERR -2
#define SHMOPEN_ERR          -3
#define FTRUNCATE_ERR        -4
#define MMAP_ERR             -5
#define PIPE_ERR             -6
#define FORK_ERR             -7
#define EXCECVE_ERR          -8
#define DUP_ERR              -9

size_t shmWrite(char * shmBuffer, char * str, sem_t * mutex);
char * createSHM(char * name, size_t size);
void sendSlaveTask(char * path, int fd);
void createSlaves(int * readEndpoints, int * writeEndpoints, int amount);
void killSlave(int * readfds, int * writefds, int slave);
int Dup(int oldfd);

int main(int argc, char * argv[]) {

    setvbuf(stdout, NULL, _IONBF, 0);
    
    if (argc < 2) {
        
        perror("Insuficient arguments");
        exit(INSUFICIENT_ARGS_ERR);
    }
    char * shmAddr = createSHM(SHMNAME, SHMSIZE);
    char * shmBuffer = shmAddr + sizeof(sem_t);
    sem_t * mutex = (sem_t *) shmAddr;
    size_t offset = 0;
    sem_init(mutex, 1, 0);
    
    printf("%s\n", SHMNAME);
    sleep(2); 

    int writefds[SLAVESQTY] = {[0 ... SLAVESQTY-1] = -1};
    int readfds[SLAVESQTY] = {[0 ... SLAVESQTY-1] = -1};
    int slaveTasks[SLAVESQTY] = {0};
    int slaveCompletedTasks[SLAVESQTY] = {0};

    int slaves = SLAVESQTY;

    if (argc < SLAVESQTY) {
        slaves = argc - 1;
    }
    createSlaves(readfds, writefds, slaves);

    int filesAssigned = 0;
    int initialAssign = INITIAL_TASKS;

    if (argc - 1 < slaves * INITIAL_TASKS) {
        initialAssign = 1;
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

            if (readfds[i] != -1) {
                FD_SET(readfds[i], &set);
            }
        }
        select(maxFD + 1, &set, NULL, NULL, NULL);

        for (int i = 0; i < SLAVESQTY; i++) {

            if (readfds[i] != -1 && FD_ISSET(readfds[i], &set)) {
                FD_CLR(readfds[i], &set);
                ssize_t bytesread = read(readfds[i], resultBuffer, BUFFER_SIZE);

                if (bytesread < 0) {
                    perror("Error reading result from slave");
                    exit(ORDINARY_ERROR);
                }
                size_t written = 0;

                while (written < (size_t)bytesread) {

                    int temp = shmWrite(shmBuffer + offset, resultBuffer + written, mutex);
                    written += temp + 1;
                    filesProcessed++;
                    offset += temp;
                    slaveTasks[i]--;
                    slaveCompletedTasks[i]++;
                }
                if (filesAssigned < argc - 1) {
                    if (!slaveTasks[i]) {
                        sendSlaveTask(argv[++filesAssigned], writefds[i]);
                        slaveTasks[i]++;
                    }
                } else {
                    if (slaveCompletedTasks[i] == initialAssign) {
                        killSlave(readfds, writefds, i);
                        slaves--;
                    }
                }
                i = 0;
            }
        }
    }

    shmBuffer[offset] = 0;
    sem_post(mutex);
    int resultfd = creat("./results.txt", 0600);
    write(resultfd, shmBuffer, offset);
    shm_unlink(SHMNAME);
    exit(EXIT_SUCCESS);
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

    if (shmMemFd < 0) {
        perror("shm_open");
        exit(SHMOPEN_ERR);
    }
    if (ftruncate(shmMemFd, size) < 0) {
        exit(FTRUNCATE_ERR);
    }
    char * shmAddr;
    shmAddr = mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmMemFd, 0);

    if (shmAddr == MAP_FAILED) {
        perror("mmap");
        exit(MMAP_ERR);
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
        
        if (pipe(taskpipefd) < 0) {
            perror("pipe");
            exit(PIPE_ERR);
        }
        if (pipe(anspipefd) < 0) {
            perror("pipe");
            exit(PIPE_ERR);
        }
        cpid = fork();

        if (cpid < 0) {
            perror("fork");
            exit(FORK_ERR);
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
            char * args1[] = {"./slave", NULL};
            int err1 = execve(args1[0], args1, NULL);

            if (err1 < 0) {
                perror("execve");
                exit(EXCECVE_ERR);
            }
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
    
    if (newfd < 0) {
        perror("dup");
        exit(DUP_ERR);
    }

    return newfd;
}