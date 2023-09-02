#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080

volatile sig_atomic_t RUNNING = 1;
void signal_handler(int sig) {
    RUNNING = 0;
    exit(1);
}

void close_sse(int client_fd) {
    char*   response = "event: CLOSE_SSE\n\n";
    ssize_t result   = write(client_fd, response, strlen(response));
}

void send_sse(int client_fd, int* count) {
    char response[512];

    snprintf(response, sizeof(response),
             "event: JS_ELEMENT_RUN\ndata: {\"target\": \"#target\","
             "\"positions\": [0, 3],"
             "\"callfront\": %d}\n\n",
             (*count)++);

    write(client_fd, response, strlen(response));
}

int setup_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {0};

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) return -1;
    if (listen(server_fd, 5) == -1) return -1;

    return server_fd;
}

int server() {
    int server_fd = setup_server();
    int count     = 0;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    signal(SIGINT, signal_handler);

    while (RUNNING) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        int flags     = fcntl(client_fd, F_GETFL, 0);

        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

        char    request[1024];
        ssize_t request_size = read(client_fd, request, sizeof(request));

        printf("request: %s\n", request);

        if (request_size <= 0 || !strstr(request, "GET /ooga_booga")) {
            close(client_fd);
            continue;
        }

        printf("see GET /ooga_booga\n");

        char response_headers[512];
        snprintf(response_headers, sizeof(response_headers),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/event-stream\r\n"
                 "Cache-Control: no-cache\r\n"
                 "Connection: keep-alive\r\n"
                 "Access-Control-Allow-Origin: *\r\n\r\n");

        write(client_fd, response_headers, strlen(response_headers));

        while (RUNNING && count < 10) {
            send_sse(client_fd, &count);
            sleep(1);
        }

        close_sse(client_fd);
        close(client_fd);
    }

    close(server_fd);

    return 0;
}
