#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <unistd.h> // write, read
#include <dlfcn.h>  // dlopen, dlsym, dlclose, RTLD_*

#include "library.h" // NOTE: Custom function types

static e_func *e;
static sort_func *sort;

// NOTE: Functions stubs will be used, if library failed to load
// NOTE: Stubs are better than NULL function pointers,
//       you don't need to check for NULL before calling a function
static float e_stub(int x) {
    (void)x; // NOTE: Compiler will warn about unused parameter otherwise
    return 0.0f;
}

static int *sort_stub(int *array, size_t n) {
    (void)n; // NOTE: Compiler will warn about unused parameter otherwise
    return array;
}

static int parse_int(const char *buf, size_t len, size_t *pos, int *out) {
    while (*pos < len) {
        char c = buf[*pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            ++(*pos);
        } else {
            break;
        }
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

int main(int argc, char **argv) {
    if (argc != 3) {
		char msg[1024];
		uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s impl1.so impl2.so\n", argv[0]);
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}
    (void)argc;

    void *handles[2] = { NULL, NULL }; // указатели на сами реализации
    e_func *e_impl[2] = { NULL, NULL }; // указатели на функции e в реализации 1 и2
    sort_func *sort_impl[2] = { NULL, NULL }; // указатели на функции sort в реализации 1 и 2

    for (int i = 0; i < 2; ++i) {
        handles[i] = dlopen(argv[i + 1], RTLD_LOCAL | RTLD_NOW);
        if (handles[i] == NULL) {
            char msg[256];
            int lenght = snprintf(msg, sizeof(msg) - 1, "warning: failed to load library '%s'\n", argv[i + 1]);
            if (lenght > 0) { msg[lenght] = '\0'; write(STDERR_FILENO, msg, lenght); }
            continue;
        }

        e_impl[i] = dlsym(handles[i], "e");
        if (e_impl[i] == NULL) {
            char msg[256];
            int lenght = snprintf(msg, sizeof(msg) - 1, "warning: failed to find e function in '%s'\n", argv[i + 1]);
            if (lenght > 0) { msg[lenght] = '\0'; write(STDERR_FILENO, msg, lenght); }
        }

        sort_impl[i] = dlsym(handles[i], "sort");
        if (sort_impl[i] == NULL) {
            char msg[256];
            int lenght = snprintf(msg, sizeof(msg) - 1, "warning: failed to find sort function in '%s'\n", argv[i + 1]);
            if (lenght > 0) { msg[lenght] = '\0'; write(STDERR_FILENO, msg, lenght); }
        }
    }

    int cur_impl = 0;
    e = e_impl[cur_impl] ? e_impl[cur_impl] : e_stub;
    sort = sort_impl[cur_impl] ? sort_impl[cur_impl] : sort_stub;

    char buf[BUFSIZ];
    ssize_t bytes;

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        size_t len = (size_t)bytes;
        size_t pos = 0;

        if (pos >= len || buf[pos] == '\n') continue;

        int mode = 0;
        if (parse_int(buf, len, &pos, &mode) != 0) {
            const char msg[] = "error: expected command (0, 1 or 2)\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            continue;
        }

        if (!mode) {
            cur_impl = 1 - cur_impl;

            e = e_impl[cur_impl] ? e_impl[cur_impl] : e_stub;
            sort = sort_impl[cur_impl] ? sort_impl[cur_impl] : sort_stub;

            char msg[128];
            int lenght = snprintf(msg, sizeof(msg) - 1, "Switched to implementation #%d (%s)\n", cur_impl + 1, argv[cur_impl + 1]);
            if (lenght > 0) { msg[lenght] = '\0'; write(STDOUT_FILENO, msg, lenght); }
        } else if (mode == 1) {
            int x;
            if (parse_int(buf, len, &pos, &x) != 0) {
                const char msg[] = "error: expected argument for command 1\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                break;
            }

            float result = e(x);

            char out[BUFSIZ];
            int length = snprintf(out, sizeof(out) - 1, "e(%d) = %.6f\n", x, result);
            if (length > 0) {
                out[length] = '\0';
                write(STDOUT_FILENO, out, length);
            }
        } else if (mode == 2) {
            size_t n;
            if (parse_size_t(buf, len, &pos, &n) != 0) {
                const char msg[] = "error: expected size for command 2\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                break;
            }

            int *array = malloc(n * sizeof(int));
            if (!array) {
                const char msg[] = "error: malloc failed\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                break;
            }

            for (size_t i = 0; i < n; ++i) {
                if (parse_int(buf, len, &pos, &array[i]) != 0) {
                    const char msg[] = "error: expected array element\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    free(array);
                    for (int i = 0; i < 2; ++i) {
                        if (handles[i]) dlclose(handles[i]);
                    }
                    return 0;
                }
            }

            int *res = sort(array, n);

            char out[BUFSIZ];
            size_t offset = 0;
            for (size_t i = 0; i < n; ++i) {
                int lenght = snprintf(out + offset, sizeof(out) - 1 - offset, "%d%c", res[i], (i + 1 == n) ? '\n' : ' ');
                if (lenght < 0 || (size_t)lenght >= sizeof(out) - 1 - offset) break;
                offset += (size_t)lenght;
            }
            out[offset] = '\0';
            write(STDOUT_FILENO, out, offset);

            free(array);
        } else {
            const char msg[] = "error: wrong function (must be 1 or 2)\n";
            write(STDERR_FILENO, msg, sizeof(msg));
        }
    }

    for (int i = 0; i < 2; ++i) {
        if (handles[i]) dlclose(handles[i]);
    }

    return 0;
}
