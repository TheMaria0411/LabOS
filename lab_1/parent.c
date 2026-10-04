#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

static char CHILD_NAME[] = "child";

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

    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        const char msg[] = "error: failed to read full program path\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    path[len] = '\0';
    while (len > 0 && path[len - 1] != '/') --len;
    path[len] = '\0';

    int parent_to_child[2];

    if (pipe(parent_to_child) == -1) {
        const char msg[] = "error: failed to create pipe parent_to_child\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    int child_to_parent[2];

    if (pipe(child_to_parent) == -1) {
        const char msg[] = "error: failed to create pipe parent_to_child\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    const pid_t child = fork();

    switch (child) {
    case -1: { 
        const char msg[] = "error: failed ot spawn new proccess\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    case 0: {
        close(parent_to_child[1]);
        close(child_to_parent[0]);
        dup2(parent_to_child[0], STDIN_FILENO);
        close(parent_to_child[0]);
        dup2(child_to_parent[1], STDOUT_FILENO);
        close(child_to_parent[1]);

        char child_path[2048];

        strcpy(child_path, path);
        strcat(child_path, CHILD_NAME);

        char *const args[] = {CHILD_NAME, filename, NULL};
        int status = execv(child_path, args);

        if (status == -1){
            const char msg[] = "error: failed to execv\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
        }
    } break;

    default: {
        close(parent_to_child[0]);
        close(child_to_parent[1]);

        const char msg[] = "Enter number\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);

        char buf[4096];
    
        while (1) {
            ssize_t bytes = read(STDIN_FILENO, buf, sizeof(buf));

            if (bytes < 0) {
                const char msg[] ="stdin read failed\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                exit(EXIT_FAILURE);
            }

            if (bytes == 0) break;
            if (bytes == 1 && buf[0] == '\n') break;

            int32_t written = write(parent_to_child[1], buf, bytes);
            
            if (written != bytes) {
                const char msg[] = "error: failed to write\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                exit(EXIT_FAILURE);
            }
            char answer;
            if (read(child_to_parent[0], &answer, sizeof(answer)) <= 0) break;
            if (answer == 'B') break;
            if (answer == 'E'){
                const char msg[] = "unprocessed number(0 or 1)\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                continue;
            }
        }
        close(parent_to_child[1]);
        close(child_to_parent[0]);

        wait(NULL);
    } break;
    }
    return 0;
}
