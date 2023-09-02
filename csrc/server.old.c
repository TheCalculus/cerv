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
#define CLOSEIF1(VAL)                     \
    if (VAL) {                            \
        char buffer[50];                  \
        sprintf(buffer, "ln %d, %s(...)", \
                __LINE__, __func__);      \
        perror(buffer);                   \
        exit(1);                          \
    }                                     \

int RUNNING = 1;

void send_sse(int client_fd, int* count) {
    char response[512];

    snprintf(response, sizeof(response),
            "event: JS_ELEMENT_RUN\n"
            "data: {\"target\": \"#target\","
                   "\"positions\": [0, 3],"
                   "\"callfront\": %d}\n\n",
            (*count)++);

    write(client_fd, response, strlen(response));
}

void close_sse(int client_fd) {
    char* response = "event: CLOSE_SSE\n";
    write(client_fd, response, strlen(response));
}

void handle_sigint() {
    perror("(SIGINT)");
    CLOSEIF1(1);
}

int server() {
    signal(SIGINT, handle_sigint);

    struct sockaddr_in server_addr, client_addr;
    int                server_fd,   client_fds[MAX_CLIENTS],
                       count,       kq/*ueue*/, result, client_count;
    struct kevent      event,       tevent;


    CLOSEIF1((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1);

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    CLOSEIF1(bind  (server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1);
    CLOSEIF1(listen(server_fd, 5) == -1);

    CLOSEIF1((kq = kqueue()) == -1);

    EV_SET(&event, server_fd, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE, 0, NULL);
    CLOSEIF1((result = kevent(kq, &event, 1, NULL, 0, NULL)) == -1);

    uint64_t  bytes_written = 0;

    while (RUNNING) {
        CLOSEIF1((result = kevent(kq, NULL, 0, &tevent, 1, NULL)) == -1);

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
                int       client_fd  = 0;

                CLOSEIF1((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len)) == -1);

                EV_SET(&event, client_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                CLOSEIF1((kevent(kq, &event, 1, NULL, 0, NULL)) == -1);

                int flags    = fcntl(client_fd, 0, F_GETFL, 0);
                /* nonblock */ fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

                client_fds[client_count++] = client_fd;
            }
            else if
            ((&tevent)[i].filter == EVFILT_READ) {
                char    request[1024];
                ssize_t request_len = recv(fd, request, sizeof(request), 0);

                if (request_len <= 0) {
                    printf("%d disconnected\n", fd);
                    close(fd);

                    EV_SET(&event, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                    CLOSEIF1(kevent(kq, &event, 1, NULL, 0, NULL) == -1);

                    for (int j = 0; j < client_count; j++) {
                        if (client_fds[j] != fd) continue;
                    
                        for (int k = j; k < client_count - 1; k++)
                            client_fds[k] = client_fds[k + 1];

                        client_count--;
                        break;
                    }
                } else { printf("%ld bytes read\n", request_len); }
            }
            else if
            ((&tevent)[i].filter == EVFILT_WRITE) {
                off_t offset = (off_t)(&tevent)[i].udata;
                off_t length = 0;

                char response_headers[512];

                snprintf(response_headers, sizeof(response_headers),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/event-stream\r\n"
                        "Cache-Control: no-cache\r\n"
                        "Connection: keep-alive\r\n"
                        "Access-Control-Allow-Origin: *\r\n\r\n");

                if ((length = send(fd, response_headers, strlen(response_headers), MSG_DONTWAIT)) == 0) {
                    bytes_written += length;
                    printf("wrote %lld bytes, %lld total\n", length, bytes_written);
                }

                if (errno == EAGAIN) {
                    EV_SET(&event, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, (void*)(offset + length));
                    kevent(kq, &event, 1, NULL, 0, NULL);
                }
            }
        }

//      while (RUNNING) {
//          send_sse(client_fd, &count);
//          RUNNING = count < 10;
//          if (!RUNNING) close_sse(client_fd);
//          sleep(1);
//      }

        for (int j = 0; j < client_count; j++)
            close(client_fds[j]);
    }

    return 0;
}
