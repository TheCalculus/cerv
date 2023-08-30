#ifndef SSE_H
#define SSE_H

int  server();
void send_sse(int client_fd, int* count);

extern int server_fd;
extern int client_fd;
extern int count;

#endif
