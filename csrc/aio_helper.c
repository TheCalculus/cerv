#include <aio.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "aio_helper.h"

struct aiocb* ioreqs  = NULL;
size_t        numreqs = 5;
size_t        inxreqs = 0;

struct aiocb* init_ioreqs() {
    ioreqs = calloc(numreqs, sizeof(struct aiocb));
    return ioreqs;
}

struct aiocb* generate_ioreq(int fd) {
    memcpy(&ioreqs + inxreqs++, new_aio(fd), sizeof(struct aiocb));
    return *(&ioreqs + inxreqs);
}

struct aiocb* new_aio(int fd) {
    struct aiocb* server_aiocb = (struct aiocb*)malloc(sizeof(struct aiocb));

    memset(server_aiocb, 0, sizeof(struct aiocb)); // any reason to use bzero() over this?

    int          BUFSIZE = 0;
    unsigned int sof_int = sizeof(BUFSIZE); // sizeof_int

    getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&BUFSIZE, &sof_int);

    server_aiocb->aio_buf    = malloc(BUFSIZE + 1);
    server_aiocb->aio_fildes = fd;
    server_aiocb->aio_nbytes = BUFSIZE;
    server_aiocb->aio_offset = 0;

    while (aio_error(server_aiocb) == EINPROGRESS);

    return server_aiocb;
}

ssize_t async_read(int fd, char* buffer, size_t count) {
    struct aiocb* cbp = generate_ioreq(fd);

    cbp->aio_buf      = buffer;
    cbp->aio_nbytes   = count;
    
    int retval        = aio_read(cbp); 
    return retval;
};

// TODO: minimise code repetition, should be trivial

ssize_t async_write(int fd, char* buffer, size_t count) {
    struct aiocb* cbp = generate_ioreq(fd);
   
    cbp->aio_buf      = buffer;
    cbp->aio_nbytes   = count;
   
    int retval        = aio_write(cbp); 
    return retval;
};

void free_ioreqs() {
    for (int i = 0; i < inxreqs; i++) { free(&ioreqs[i]); }
    free(ioreqs);
};
