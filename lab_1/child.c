#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int is_prime(int n){
    if (n < 2)
        return 0;
    for (int i = 2; i <= n / i; ++i) {
        if (n % i == 0)
            return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    char buf[4096];
    ssize_t bytes;
    if (argc != 2) {
        const char msg[] = "error: failed to read argv\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    int file = open(argv[1],  O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (file == -1) {
        const char msg[] = "error: failed to open file\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1 ))) {
        if (bytes < 0) {
            const char msg[] = "stdin reading failed\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        buf[bytes] = '\0';
        char *end;
        long val = strtol(buf, &end, 10);
        int n = (int)val;

        if (n < 0) {
            char answer = 'B';
            write(STDOUT_FILENO, &answer, sizeof(answer));
            break;
        }

        if (is_prime(n)) {
            char answer = 'B';
            write(STDOUT_FILENO, &answer, sizeof(answer));
            break;
        }

        if (n == 0 || n == 1) {
            char answer = 'E';
            write(STDOUT_FILENO, &answer, sizeof(answer) );
            continue;
        }

        int32_t written = write(file, buf, bytes);
        if (written != bytes) {
            const char msg[] = "error: failed to write\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }

        char answer = 'C';
        write(STDOUT_FILENO, &answer, sizeof(answer));
    }
    close(file);
    return 0;
}
