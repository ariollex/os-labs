#include <stdlib.h>
#include <unistd.h>
#define BUFSIZ 4096

int main(void) {
    char buf[BUFSIZ];
	ssize_t bytes;

    while((bytes = read(STDIN_FILENO, buf, sizeof(buf)))) {
        if (bytes < 0) {
            const char msg[] = "Error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        } else if (buf[0] == '\n') {
            break;
        }
        int i = 0;
        char last = '\n';
        while (i < bytes && buf[i] != '\n') last = buf[i++];
        if (last == '.' || last == ';') {
            const char zero = 0;
            write(STDOUT_FILENO, buf, bytes);
            write(STDERR_FILENO, &zero, sizeof(zero));
        } else {
            const char msg[] = "Error: string do not match the rule\n";
            write(STDERR_FILENO, msg, sizeof(msg));
        }
    }
    return 0;
}
