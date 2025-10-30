#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/fcntl.h>
#include <stdio.h>
#define SHM_SIZE 4096

const char SHM_NAME[] = "shm";
const char SEM_NAME[] = "sem";

int main(void) {
    int shm = shm_open(SHM_NAME, O_RDWR, 0);
	if (shm == -1) {
		const char msg[] = "error: failed to open SHM\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(EXIT_FAILURE);
	}

	char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if (shm_buf == MAP_FAILED) {
		const char msg[] = "error: failed to map SHM\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(EXIT_FAILURE);
	}

	sem_t *sem = sem_open(SEM_NAME, O_RDWR);
	if (sem == SEM_FAILED) {
		const char msg[] = "error: failed to open semaphore\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(EXIT_FAILURE);
	}

    bool running = true;
	while (running) {
		sem_wait(sem);

		uint32_t *length = (uint32_t *)shm_buf;
		char *text = shm_buf + sizeof(uint32_t);

		if (*length == UINT32_MAX) {
			running = false;
		} else if (*length > 0 && *length < SHM_SIZE) {
			ssize_t n = *length;
			int i = 0, count = 0;
			char last = '\n';
			while (i < n && text[i] != '\n') last = text[i++];

			if (last == '.' || last == ';') {
				const char zero = 0;
				count = write(STDOUT_FILENO, text, n);
				write(STDERR_FILENO, &zero, sizeof(zero));
			} else {
				const char msg[] = "Error: string do not match the rule\n";
				write(STDERR_FILENO, msg, sizeof(msg) - 1);
			}
            if (count < 0) count = 0;   
			*length = count + SHM_SIZE;    
		}
		sem_post(sem);
	}

	sem_close(sem);
	munmap(shm_buf, SHM_SIZE);
	close(shm);

    return 0;
}
