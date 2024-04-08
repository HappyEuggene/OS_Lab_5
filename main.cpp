#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

int f(int x) {
    sleep(1);
    return x % 2;
}

int g(int x) {
    sleep(3);
    return x % 3 ? 0 : 1;
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <integer value>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int x = atoi(argv[1]);
    int fd1[2], fd2[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd1) < 0) {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd2) < 0) {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(fd1[0]);
        int result = f(x);
        write(fd1[1], &result, sizeof(result));
        close(fd1[1]);
        exit(0);
    } else if (pid1 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(fd2[0]);
        int result = g(x);
        write(fd2[1], &result, sizeof(result));
        close(fd2[1]);
        exit(0);
    } else if (pid2 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    close(fd1[1]);
    close(fd2[1]);

    fd_set readfds;
    int max_fd = fd1[0] > fd2[0] ? fd1[0] : fd2[0];
    int result, logical_and = 1, read_count = 0;

    while (read_count < 2) {
        FD_ZERO(&readfds);
        FD_SET(fd1[0], &readfds);
        FD_SET(fd2[0], &readfds);

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(fd1[0], &readfds)) {
            if (read(fd1[0], &result, sizeof(result)) > 0) {
                logical_and &= result;
                read_count++;
            }
        }

        if (FD_ISSET(fd2[0], &readfds)) {
            if (read(fd2[0], &result, sizeof(result)) > 0) {
                logical_and &= result;
                read_count++;
            }
        }
    }

    printf("Result of f(x) && g(x): %d\n", logical_and);

    close(fd1[0]);
    close(fd2[0]);

    return EXIT_SUCCESS;
}
