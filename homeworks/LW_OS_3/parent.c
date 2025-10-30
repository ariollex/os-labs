#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdbool.h>
#include <errno.h>

#define SHM_SIZE 4096

const char SHM_NAME[] = "shm";
const char SEM_NAME[] = "sem";
#define CHILD_PROGRAM_NAME "child"

int main(int argc, char** argv) {
	if (argc != 2) {
		char msg[1024];
		uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
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

    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);

	int shm = shm_open(SHM_NAME, O_RDWR, 0600);
	if (shm == -1 && errno != ENOENT) {
		const char msg[] = "error: failed to open SHM\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(EXIT_FAILURE);
	}
	
	shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (shm == -1) {
		const char msg[] = "error: failed to create SHM\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(EXIT_FAILURE);
	}

	if (ftruncate(shm, SHM_SIZE) == -1) {
		const char msg[] = "error: failed to resize SHM\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(EXIT_FAILURE);
	}

	char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if (shm_buf == MAP_FAILED) {
		const char msg[] = "error: failed to map SHM\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(EXIT_FAILURE);
	}

	sem_t *sem = sem_open(SEM_NAME, O_RDWR | O_CREAT | O_TRUNC, 0600, 1);
	if (sem == SEM_FAILED) {
		const char msg[] = "error: failed to create semaphore\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(EXIT_FAILURE);
	}

    const pid_t child = fork();

    if (!child) {
        char path[1024];
        snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CHILD_PROGRAM_NAME);
        char* const args[] = {path, NULL};

        int32_t status = execv(path, args);
        if (status == -1) {
            const char msg[] = "Error: failed to exec into new executable image\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            _exit(EXIT_FAILURE);
        }
	} else if (child == -1) {
		const char msg[] = "error: failed to fork\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(EXIT_FAILURE);
    }

    bool running = true;
	while (running) {
		char buf[SHM_SIZE - sizeof(uint32_t)];
		ssize_t bytes = read(STDIN_FILENO, buf, sizeof(buf));
		if (bytes == -1) {
			const char msg[] = "error: failed to read from standard input\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			_exit(EXIT_FAILURE);
		}
		sem_wait(sem);
		uint32_t *length = (uint32_t *)shm_buf;
		char *text = shm_buf + sizeof(uint32_t);
		if (bytes > 0) {
			*length = (uint32_t)bytes;
            memcpy(text, buf, bytes);
            sem_post(sem);

			while(true) {
                sem_wait(sem);
                uint32_t len = *length;
                if (len >= SHM_SIZE) { 
                    ssize_t new_bytes = len - SHM_SIZE;
                    *length = 0;
                    sem_post(sem);
                    if (new_bytes != bytes) write(fd, buf, bytes);
                    break;
                }
                sem_post(sem);
            }
		} else {
			*length = UINT32_MAX;
            sem_post(sem); 
			running = false;
		}

	}

    waitpid(child, NULL, 0);

	sem_unlink(SEM_NAME);
	sem_close(sem);
	munmap(shm_buf, SHM_SIZE);
	shm_unlink(SHM_NAME);
	close(shm);

    close(fd);
    return 0;
}
