#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>

#include "utils.c"

#define UINT128_MAX  ((__uint128_t)-1)
#define INT128_MAX   ((__int128_t)(UINT128_MAX >> 1))
#define INT128_MIN   (-INT128_MAX - 1)
#define WORD 34

typedef struct {
    int fd;
    int status;
    off_t start;
    off_t end;
    size_t chunk;
    __int128_t amount;
    __int128_t count;
} ThreadArgs;

static void *work(void *_args)
{
    ThreadArgs *args = (ThreadArgs *)_args;
    if (args->fd == -1) {
        args->status = -1;
        return NULL;
    }

    char *buf = (char*)malloc(args->chunk);
    char *number = (char*)malloc(WORD);
    if (!buf || !number) {
        args->status = 1;
        free(buf); free(number); return NULL;
    }

    size_t index = 0;
    off_t pos = args->start;
    int skip_start = 0;

    if (args->start > 0) {
        unsigned char prev;
        ssize_t prev_byte = pread(args->fd, &prev, 1, args->start - 1);
        if (prev_byte == 1 && !isspace(prev)) skip_start = 1;
    }

    while (pos < args->end + WORD) {
        size_t safe_chunk = (args->end + WORD < pos + args->chunk ? args->end + WORD - pos : args->chunk);
        if (safe_chunk <= 0) { free(number); free(buf); return NULL; }

        ssize_t bytes = pread(args->fd, buf, safe_chunk, pos);
        if (bytes < 0) {
            args->status = 1;
            free(number); free(buf); return NULL;
        }
    
        int i = 0;
        do {
            unsigned char c = buf[i];

            if (skip_start) {
                if (!isspace(c)) continue;
                skip_start = 0;
                continue;
            }

            if (isspace(c) || !bytes) {
                if (index == 0) continue;
                off_t started = pos + i - index;
                if (started < args->end) {
                    number[index] = '\0';
                    char *endptr = NULL;
                    __int128_t dec = toDecimal(number, &endptr, 16);
                    if (dec >= 0 && (args->amount > INT128_MAX - dec) || (dec < 0 && (args->amount < INT128_MIN - dec)) || (*endptr != '\0')) {
                        args->status = (*endptr != '\0') ? 3 : 2;
                        free(number); free(buf); return NULL;
                    }
                    args->amount += dec;
                    ++args->count;
                }
                index = 0;
            } else {
                if (index < WORD - 2) {
                    number[index++] = c;
                } else {
                    args->status = 3;
                    free(number); free(buf); return NULL;
                }
            }
        } while (++i < bytes);

        if (!bytes) return NULL;
        pos += bytes;
    }
}

int main(int argc, char** argv) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    size_t n_threads;

    size_t chunk;
    const char *path = NULL;
	if (argc != 6) {
		char msg[1024];
		uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s -m CHUNK_SIZE -t THREADS_NUMBER filename\n", argv[0]);
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}

    for(int i = 1; i < argc; ++i) {
        if ((argv[i][0] == '-') && argv[i][1] == 'm' && argv[i][2] == '\0') {
            char* endptr;
            chunk = strtol(argv[++i], &endptr, 10);
            if (*endptr != '\0' || chunk < 1) {
                perror("Error: wrong CHUNK_SIZE!");
                return -1;
            }
        } else if ((argv[i][0] == '-') && argv[i][1] == 't' && argv[i][2] == '\0') {
            char* endptr;
            n_threads = strtol(argv[++i], &endptr, 10);
            if (*endptr != '\0' || n_threads < 1) {
                perror("Error: wrong n_threads!");
                return -1;
            }
        } else if (!path) {
            path = argv[i];
        }
    }

    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    ThreadArgs *thread_args = malloc(n_threads*sizeof(ThreadArgs));

    int fd = open(path, O_RDONLY);
    struct stat fileinfo;
    if (fstat(fd, &fileinfo) == -1) {
        perror("Error while fstat");
        return -1;
    };
    off_t size = fileinfo.st_size;

    for(int i = 0; i < n_threads; ++i) {
        off_t start = (size * i) / n_threads;
        off_t end = (size * (i + 1)) / n_threads;
        thread_args[i] = (ThreadArgs){
            .fd = fd,
            .status = 0,
            .start = start,
            .end = end,
            .chunk = chunk,
            .amount = 0,
            .count = 0,
        };
        pthread_create(&threads[i], NULL, work, &thread_args[i]);
    }

    for(int i = 0; i < n_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    __int128_t amount = 0;
    __int128_t count = 0;
    int error = 0;
    for(int i = 0; i < n_threads; ++i) {
        int status = thread_args[i].status;
        if (status) error = 1;
        if (status == 1) {
            const char msg[] = "Error: failed to read from file\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            break;
        } else if (status == -1) {
            const char msg[] = "Error: failed to open file\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            break;
        } else if (status == 2 || (thread_args[i].amount >= 0 && (amount > INT128_MAX - thread_args[i].amount) || (thread_args[i].amount < 0 && (amount < INT128_MIN - thread_args[i].amount )))) {
            const char msg[] = "Error: amount (int128) overflow!\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            break;
        } else if (status == 3) {
            const char msg[] = "Error: failed to read number\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            break;
        }
        amount += thread_args[i].amount;
        count += thread_args[i].count;
    }

    if (!error) {
        char msg[1024];
        char res[WORD];
        int128ToString(res, amount / count);
        uint32_t len = snprintf(msg, sizeof(msg), "Arithmetic mean: %s\n", res);
        write(STDOUT_FILENO, msg, len);
    }

    free(thread_args);
    free(threads);
    close(fd);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double ms = (end.tv_sec - start.tv_sec) * 1e3 + (end.tv_nsec - start.tv_nsec) / 1e6;
    printf("Elapsed time: %.3f ms\n", ms);
}
