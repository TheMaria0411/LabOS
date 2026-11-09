#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>

#define SHM_SIZE 4096

typedef struct {
    uint32_t length;
    char answer;
    char buf[SHM_SIZE - sizeof(uint32_t) - sizeof(char)];
} SharedData;

static void str_to_int(int n, char *out) {
    char buf[16];
    int t = 0, i = 0;
    if (n == 0) buf[t++] = '0';

    while (n > 0) {
        buf[t++] = '0' + n % 10;
        n /= 10; 
    }
    while (t > 0) out[i++] = buf[--t];
    out[i] = '\0';
}

int main() {
    char filename[1024];

    const char filename_msg[] = "Enter filename: ";
    write(STDOUT_FILENO, filename_msg, sizeof(filename_msg) - 1);
    ssize_t filename_len = read(STDIN_FILENO, filename, sizeof(filename) - 1);

    if (filename_len <= 0) {
        const char msg[] = "error: filename read failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    if (filename[filename_len - 1] == '\n') filename[filename_len - 1] = '\0';
    else filename[filename_len] = '\0';

    char id[20], shm_name[64], sem_name1[64], sem_name2[64];
    str_to_int(getpid(), id);

    strcpy(shm_name, "/lab_shm_");
    strcat(shm_name, id);
    
    strcpy(sem_name1, "/lab_sem1_");
    strcat(sem_name1, id);
    
    strcpy(sem_name2, "/lab_sem2_");
    strcat(sem_name2, id);

    int shm = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (shm == -1 || ftruncate(shm, SHM_SIZE) == -1) {
        const char msg[] = "error: shared memory open failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    SharedData *data = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0); //маппинг памяти - подключает объект к адресному пространству процесса
    if (data == MAP_FAILED) {
        const char msg[] = "error: memory mapping failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    sem_t *sem1 = sem_open(sem_name1, O_CREAT | O_EXCL, 0600, 0);
    sem_t *sem2 = sem_open(sem_name2, O_CREAT | O_EXCL, 0600, 0);
    if (sem1 == SEM_FAILED || sem2 == SEM_FAILED) {
        const char msg[] = "error: semaphore open failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    };

    char path[1024], child_path[2048];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        const char msg[] = "error: failed to read full program path\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    path[len] = '\0';
    while (len > 0 && path[len - 1] != '/') --len;
    path[len] = '\0';

    strcpy(child_path, path);
    strcat(child_path, "child");

    pid_t child = fork();
    switch (child) {
    case -1: { 
        const char msg[] = "error: failed ot spawn new proccess\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    case 0: {
        char *args[] = {"child", filename, shm_name, sem_name1, sem_name2, NULL};
        execv(child_path, args);
        _exit(EXIT_FAILURE);
    } break;

    default: {
        const char msg[] = "Enter number\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);

        char buf[sizeof(data->buf) - 1];

        while (1) {
            ssize_t bytes = read(STDIN_FILENO, buf, sizeof(buf));
            if (bytes < 0) {
                const char msg[] ="stdin read failed\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                exit(EXIT_FAILURE);
            }

            if (bytes == 0) break;
            if (bytes == 1 && buf[0] == '\n') break;

            data->length = bytes;
            memcpy(data->buf, buf, bytes);

            sem_post(sem1);
            sem_wait(sem2);

            if (data->answer == 'B') break;
            if (data->answer == 'E') {
                const char msg[] = "unprocessed number(0 or 1)\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                continue;
            }
        }
        wait(NULL);
        sem_close(sem1);
        sem_close(sem2);
        sem_unlink(sem_name1);
        sem_unlink(sem_name2);
        munmap(data, SHM_SIZE);
        close(shm);
        shm_unlink(shm_name);
        } break;
    }
    return 0;
}
