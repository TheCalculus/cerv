#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT          8080
#define MAX_CLIENTS   100

#define ASSERTNOT(VAL)                                  \
    if (VAL) {                                         \
        char buffer[50];                               \
        snprintf(buffer, sizeof(buffer),               \
                "ln %d, %s(...)", __LINE__, __func__); \
        perror(buffer);                                \
        exit(1);                                       \
    }

int RUNNING = 1;

void handle_sigint(int signum) { RUNNING = 0; }
void handle_disconnect(int fd, int kq, struct kevent* event, int* client_fds, int* client_count) {
    printf("%d disconnected\n", fd);
    close(fd);

    EV_SET   (&event, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    ASSERTNOT(kevent(kq, event, 1, NULL, 0, NULL) == -1);

    for (int j = 0; j < *client_count; j++) {
        if (client_fds[j] != fd) continue;

        for (int k = j; k < *client_count - 1; k++)
            client_fds[k] = client_fds[k + 1];

        (*client_count)--;
        break;
    }
}

int server() {
    signal(SIGINT, handle_sigint);

    struct sockaddr_in server_addr , client_addr;
    struct kevent      event       , tevent;
    int                server_fd   , client_fds[MAX_CLIENTS],
                       kq          , result , client_count = 0;

    ASSERTNOT((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1);

    memset  (&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    ASSERTNOT(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1);
    ASSERTNOT(listen(server_fd, 5) == -1);

    ASSERTNOT((kq = kqueue()) == -1);

    EV_SET   (&event, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    ASSERTNOT(kevent(kq, &event, 1, NULL, 0, NULL) == -1);

    while (RUNNING) {
        ASSERTNOT((result = kevent(kq, NULL, 0, &tevent, 1, NULL)) == -1);

        for (int i = 0; i < result; i++) {
            int fd = (int)((&tevent)[i].ident);

            if ((&tevent)[i].flags & EV_EOF) {
                printf("%d disconnected\n", fd);
                close(fd);
            }
            else if
            (fd == server_fd) {
                if (client_count > MAX_CLIENTS) continue;

                socklen_t client_len = sizeof(client_addr);
                int       client_fd = 0;

                ASSERTNOT((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len)) == -1);

                EV_SET   (&event, client_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                ASSERTNOT(kevent(kq, &event, 1, NULL, 0, NULL) == -1);

                int flags    = fcntl (client_fd, F_GETFL, 0);
                /* nonblock */ fcntl (client_fd, F_SETFL, flags | O_NONBLOCK);

                client_fds[client_count++] = client_fd;
            }
            else if
            ((&tevent)[i].filter == EVFILT_READ) {
                char    request[1024];
                ssize_t request_len = recv(fd, request, sizeof(request), 0);

                if (request_len <= 0) {
                    handle_disconnect(fd, kq, &event, client_fds, &client_count);
                    continue;
                }

                // this write has to happen upon successful read
                printf("%ld bytes read\n", request_len);

                const char* response = "HTTP/1.1 200 OK\r\n"
                                       "Content-Type: text/event-stream\r\n"
                                       "Cache-Control: no-cache\r\n"
                                       "Connection: keep-alive\r\n"
                                       "Access-Control-Allow-Origin: *\r\n\r\n";

                write(fd, response, strlen(response));
            }
        }
    }

    for (int j = 0; j < client_count; j++) close(client_fds[j]);
    close(server_fd);

    return 0;
}
