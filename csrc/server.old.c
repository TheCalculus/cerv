#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT          8080
#define CLOSEIF1(VAL)                     \
    if (!(VAL)) {                         \
        char buffer[50];                  \
        sprintf(buffer, "error: %d, %s",  \
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
    perror("(SIGINT)\n");
    CLOSEIF1(1);
}

int server() {
    signal(SIGINT, handle_sigint);

    struct sockaddr_in server_addr, client_addr;
    int                server_fd,   client_fd,
                       count,       kq/*ueue*/, result;
    struct kevent      event,       tevent;


    CLOSEIF1((socket(AF_INET, SOCK_STREAM, 0)) == -1);

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    CLOSEIF1(bind  (server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1);
    CLOSEIF1(listen(server_fd, 5) == -1);

    CLOSEIF1((kq = kqueue()) == -1);

    EV_SET(&event, client_fd, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE, 0, NULL);
    result = kevent(kq, &event, 1, NULL, 0, NULL);

    CLOSEIF1(result == -1);
    CLOSEIF1(event.flags & EV_ERROR);

    socklen_t client_len = sizeof(client_addr);

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
                struct sockaddr_storage server_addr_in;
                socklen_t server_len = sizeof(server_addr_in);

                CLOSEIF1((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len)) == -1);
            }
            else if
            ((&tevent)[i].filter == EVFILT_READ) {
            }
            else if
            ((&tevent)[i].filter == EVFILT_WRITE) {
            }
        }


        CLOSEIF1((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len)) == -1);
        fcntl   (client_fd, F_SETFD, (fcntl(client_fd, F_GETFD) | O_NONBLOCK));

        char    request[1024];
        ssize_t request_len = read(client_fd, request, sizeof(request));

        if (request_len < 0 || !strstr(request, "GET /ooga_booga")) continue;

        char response_headers[512];

        snprintf(response_headers, sizeof(response_headers),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/event-stream\r\n"
                "Cache-Control: no-cache\r\n"
                "Connection: keep-alive\r\n"
                "Access-Control-Allow-Origin: *\r\n\r\n");

        write(client_fd, response_headers, strlen(response_headers));

        while (RUNNING) {
            send_sse(client_fd, &count);
            RUNNING = count < 10;
            if (!RUNNING) close_sse(client_fd);
            sleep(1);
        }

        close(client_fd);
    }

    return 0;
}
