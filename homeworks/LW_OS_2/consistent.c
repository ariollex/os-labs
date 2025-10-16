#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include "utils.c"
#include <time.h>
#define UINT128_MAX ((__uint128_t)-1)
#define WORD 33

int main(int argc, char** argv) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    size_t CHUNK_SIZE;
    const char *path = NULL;
	if (argc != 4) {
		char msg[1024];
		uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s -m CHUNK_SIZE filename\n", argv[0]);
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}

    for(int i = 1; i < argc; ++i) {
        if ((argv[i][0] == '-') && argv[i][1] == 'm' && argv[i][2] == '\0') {
            char* endptr;
            CHUNK_SIZE = strtol(argv[++i], &endptr, 10);
            if (*endptr != '\0') {
                perror("Error: wrong CHUNK_SIZE!");
                return -1;
            }
        } else if (!path) {
            path = argv[i];
        }
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("Error: failed to open file");
        return -1;
    }

    __uint128_t amount = 0;
    __uint128_t count = 0;

    char buf[CHUNK_SIZE];
    ssize_t bytes;
    
    char* number = (char*)malloc(WORD);
    int index = 0;

    do {
        bytes = read(fd, buf, CHUNK_SIZE);
        if (bytes < 0) {
            const char msg[] = "Error: failed to read from file\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }
        int i = 0;
        do {
            unsigned char c = buf[i];

            if (isspace(c) || !bytes) {
                if (index == 0) continue;
                number[index++] = '\0';
                char* endptr;
                __uint128_t dec = toDecimal(number, &endptr, 16);
                if (amount > UINT128_MAX - dec || *endptr != '\0') {
                    if (amount > UINT128_MAX - dec) printf("Error: amount (int128) overflow!");
                    if (*endptr != '\0') perror("Error: failed to read number");
                    free(number);
                    close(fd);
                    return -1;
                }
                amount += dec;
                ++count;
                index = 0;
            } else {
                if (index < WORD - 1) {
                    number[index++] = c;
                } else {
                    printf("Error: failed to read number");
                    free(number); close(fd); return -1;
                }
            }
        } while (++i < bytes);
    } while (bytes);

    char msg[1024];
    char res[64];
    int128ToString(res, amount / count);
	uint32_t len = snprintf(msg, sizeof(msg), "Arithmetic mean: %s\n", res);
	write(STDOUT_FILENO, msg, len);
    free(number);
    close(fd);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double ms = (end.tv_sec - start.tv_sec) * 1e3 + (end.tv_nsec - start.tv_nsec) / 1e6;
    printf("Elapsed time: %.3f ms\n", ms);
}
