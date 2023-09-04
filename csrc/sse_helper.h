#ifndef SSE_H
#define SSE_H

#define MAX_CLIENTS 100

int         server            ();
void        send_sse          (int fd);
void        send_sse_all      ();
static void handle_sigint     (int signum);
static void handle_disconnect (int fd);

extern struct kevent  event , tevent;
extern int            COUNT , server_fd , client_fds[MAX_CLIENTS],
                      kq    , result    , client_count;

#endif
