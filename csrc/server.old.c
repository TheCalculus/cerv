#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080

int client_fd, server_fd, count = 0;
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

int server() {
    struct sockaddr_in server_addr, client_addr;

    socklen_t client_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) exit(1);

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) exit(1);
    if (listen(server_fd, 5) == -1) exit(1);

    while (RUNNING) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) exit(1);

        fcntl(client_fd, F_SETFD, (fcntl(client_fd, F_GETFD) | O_NONBLOCK));

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
