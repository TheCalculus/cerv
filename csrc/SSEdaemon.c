#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "SSE.h"

#define SSE_PATH "/ooga_booga"

static void daemonise() {
    pid_t pid;

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP,  SIG_IGN);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

    int devnull = open("/dev/null", O_RDWR);
    if (devnull == -1) exit(EXIT_FAILURE);

    dup2(devnull, STDIN_FILENO);
    dup2(devnull, STDOUT_FILENO);
    dup2(devnull, STDERR_FILENO);
   
    if (devnull > STDERR_FILENO) close(devnull);
}

int main() {
    daemonise();
    server();

    int daemon = 0;
    while (daemon++ < 10) {
        send_sse(client_fd, &count);
    }

    return 0;
}
