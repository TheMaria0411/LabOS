#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <semaphore.h>

#define SHM_SIZE 4096

typedef struct {
    uint32_t length;
    char answer;
    char buf[SHM_SIZE - sizeof(uint32_t) - sizeof(char)];
} SharedData;

int is_prime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i <= n / i; i++)
        if (n % i == 0) return 0;
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        const char msg[] = "error: failed to read argv\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    int file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (file == -1) {
        const char msg[] = "error: failed to open file\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    int shm = shm_open(argv[2], O_RDWR, 0600);

    SharedData *data = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

    if (data == MAP_FAILED) {
        const char msg[] = "error: memory mapping failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    sem_t *sem1 = sem_open(argv[3], 0);
    sem_t *sem2 = sem_open(argv[4], 0);

    if (sem1 == SEM_FAILED || sem2 == SEM_FAILED) {
        const char msg[] = "error: semaphore open failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    while (1) {
        sem_wait(sem1);

        if (data->length == UINT32_MAX) break;

        data->buf[data->length] = '\0';
        int n = strtol(data->buf, NULL, 10);

        if (n < 0 || is_prime(n)) {
            data->answer = 'B';
            sem_post(sem2);
            break;
        }

        if (n == 0 || n == 1) {
            data->answer = 'E';
            sem_post(sem2);
            continue;
        }

        if (write(file, data->buf, data->length) != data->length) {
            const char msg[] = "error: write failed\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }

        data->answer = 'C';
        sem_post(sem2);
    }
    sem_close(sem1);
    sem_close(sem2);
    munmap(data, SHM_SIZE);
    close(shm);
    close(file);
    return 0;
}
