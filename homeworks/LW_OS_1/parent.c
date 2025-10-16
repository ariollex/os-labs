#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#define CHILD_PROGRAM_NAME "child"

int main(int argc, char** argv) {
	if (argc != 2) {
		char msg[1024];
		uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (!fd) {
        perror("Error: failed to open file");
        return -1;
    }

	char progpath[1024];
	{
		ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
		if (len == -1) {
			const char msg[] = "Error: failed to read full program path\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
		while (progpath[len] != '/') --len;
		progpath[len] = '\0';
	}

    int parent_to_child[2];
    if (pipe(parent_to_child) == -1) {
        const char msg[] = "Error: failed to create pipe\n";
        write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
    }

    int child_to_parent[2];
    if (pipe(child_to_parent) == -1) {
        const char msg[] = "Error: failed to create pipe\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    const pid_t child = fork();

    switch(child) {
        case -1: {
                const char msg[] = "Error: failed to spawn new process\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            } 
            break;
        case 0: {
            close(parent_to_child[1]);
            close(child_to_parent[0]);

            dup2(parent_to_child[0], STDIN_FILENO);
            close(parent_to_child[0]);

            dup2(fd, STDOUT_FILENO);

            dup2(child_to_parent[1], STDERR_FILENO);
            close(child_to_parent[1]);

            char path[1024];
            snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CHILD_PROGRAM_NAME);
            char* const args[] = {path, NULL};

            int32_t status = execv(path, args);
            if (status == -1) {
                const char msg[] = "Error: failed to exec into new executable image\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        } break;
        default: {
            close(parent_to_child[0]);
            close(child_to_parent[1]);

            char buf[BUFSIZ];
            ssize_t bytes;

            while ((bytes = read(STDIN_FILENO, buf, sizeof(buf)))) {
                if (bytes < 0) {
                    const char msg[] = "Error: failed to read from stdin\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                } else if (buf[0] == '\n') {
                    break;
                }

                write(parent_to_child[1], buf, bytes);

                ssize_t new_bytes = read(child_to_parent[0], buf, sizeof(buf));
                if (new_bytes != bytes) write(STDOUT_FILENO, buf, new_bytes);
            }

            close(parent_to_child[1]);
            close(child_to_parent[0]);
            wait(NULL);
        } break;
    }
    close(fd);
    return 0;
}
