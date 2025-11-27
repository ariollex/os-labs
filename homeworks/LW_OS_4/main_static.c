#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

#include "library.h" // NOTE: Custom function types

e_func e;
sort_func sort;

static int parse_int(const char *buf, size_t len, size_t *pos, int *out) {
    while (*pos < len) {
        char c = buf[*pos];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            ++(*pos);
        } else { 
            break ;
        };
    }
    if (*pos >= len) return -1;

    int sign = 1;
    if (buf[*pos] == '+' || buf[*pos] == '-') {
        if (buf[*pos] == '-') sign = -1;
        ++(*pos);
    }

    if (*pos >= len || buf[*pos] < '0' || buf[*pos] > '9') return -1;

    int value = 0;
    while (*pos < len && buf[*pos] >= '0' && buf[*pos] <= '9') {
        value = value * 10 + (buf[*pos] - '0');
        ++(*pos);
    }

    *out = sign * value;
    return 0;
}

static int parse_size_t(const char *buf, size_t len, size_t *pos, size_t *out) {
    int tmp;
    if (parse_int(buf, len, pos, &tmp) != 0 || tmp < 0) return -1;

    *out = (size_t)tmp;
    return 0;
}

int main(void) {
    char buf[BUFSIZ];
    ssize_t bytes;

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        size_t len = (size_t)bytes;
        size_t pos = 0;

        if (pos >= len || buf[pos] == '\n') continue;

        int mode;
        if (parse_int(buf, len, &pos, &mode) != 0) {
            const char msg[] = "Error: expected command (1 or 2)\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            continue;
        }

        if (mode == 1) {
            int x;
            if (parse_int(buf, len, &pos, &x) != 0) {
                const char msg[] = "Error: expected argument for command 1\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                break;
            }

            float result = e(x);

            char out[BUFSIZ];
            int lenght = snprintf(out, sizeof(out), "e(%d) = %.6f\n", x, result);
            if (lenght > 0) write(STDOUT_FILENO, out, lenght);

        } else if (mode == 2) {
            size_t n;
            if (parse_size_t(buf, len, &pos, &n) != 0) {
                const char msg[] = "Error: expected size for command 2\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                break;
            }

            int *array = malloc(n * sizeof(int));
            if (!array) {
                const char msg[] = "Error: malloc failed\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                break;
            }

            for (size_t i = 0; i < n; ++i) {
                if (parse_int(buf, len, &pos, &array[i]) != 0) {
                    const char msg[] = "Error: expected array element\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    free(array);
                    return 0;
                }
            }

            sort(array, n);

            char out[BUFSIZ];
            size_t offset = 0;
            for (size_t i = 0; i < n; ++i) {
                int lenght = snprintf(out + offset, sizeof(out) - offset, "%d%c", array[i], (i + 1 == n) ? '\n' : ' ');
                if (lenght < 0) break;
                offset += (size_t)lenght;
            }
            write(STDOUT_FILENO, out, offset);

            free(array);
        } else {
            const char msg[] = "error: wrong function (must be 1 or 2)\n";
            write(STDERR_FILENO, msg, sizeof(msg));
        }
    }

    return 0;
}
