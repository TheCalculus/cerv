#include <aio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "aio_helper.h"

#define PORT 8080

void send_sse(int client_fd, int* count) {
    char response[512];

    snprintf(response, sizeof(response),
        "event: JS_ELEMENT_RUN\n"
        "data: {\"target\": \"#target\","
               "\"positions\": [0, 3],"
               "\"callfront\": %d}\n\n",
        (*count)++);

    async_oper(client_fd, response, strlen(response), ASYNC_WRITE);
}

void close_sse(int client_fd) {
    char* response = "event: CLOSE_SSE\n";
    async_oper(client_fd, response, strlen(response), ASYNC_WRITE);
}

int server() {
    int client_fd = 0,
        server_fd = 0,
        count     = 0,
        RUNNING   = 1;

    struct sockaddr_in server_addr, client_addr;

    socklen_t client_len = sizeof(client_addr);

    printf("setting server fd\n");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) exit(1);

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    printf("binding and listening\n");

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) exit(1);
    if (listen(server_fd, 5) == -1) exit(1);

    init_ioreqs();

    while (RUNNING) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) exit(1);

        char    request[1024];
        ssize_t request_len = async_oper(client_fd, request, sizeof(request), ASYNC_READ);

        printf("error:   %d\n", aio_error(&ioreqs[inxreqs - 1]));
        printf("request: %s\n", request);

        if (request_len > 0 && strstr(request, "GET /ooga_booga")) {
            printf("see GET /ooga_booga\n");
            char response_headers[512];

            snprintf(response_headers, sizeof(response_headers),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/event-stream\r\n"
                    "Cache-Control: no-cache\r\n"
                    "Connection: keep-alive\r\n"
                    "Access-Control-Allow-Origin: *\r\n\r\n");

            async_oper(client_fd, response_headers, strlen(response_headers), ASYNC_WRITE);

            while (RUNNING) {
                send_sse(client_fd, &count);
                RUNNING = count < 10;
                if (!RUNNING) close_sse(client_fd);
                sleep(1);
            }
        }

        close(client_fd);
    }

    return 0;
}
