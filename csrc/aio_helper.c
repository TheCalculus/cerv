#include <aio.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "aio_helper.h"

aiocbptr ioreqs  = NULL;
size_t   numreqs = 5;
size_t   inxreqs = 0;

aiocbptr init_ioreqs() {
    ioreqs = (aiocbptr)calloc(numreqs, sizeof(struct aiocb));
    return ioreqs;
}

aiocbptr add_ioreqs(aiocbptr aio) {
    ioreqs[inxreqs++] = *aio;

    if (inxreqs >= numreqs) {
        numreqs += 5;
        ioreqs = (aiocbptr)realloc(ioreqs, sizeof(struct aiocb) * numreqs);
    }

    return &ioreqs[inxreqs - 1];
}

aiocbptr generate_ioreq(int fd) {
    aiocbptr aio = new_aio(fd);
    return add_ioreqs(aio);
}

aiocbptr new_aio(int fd) {
    aiocbptr server_aiocb = (aiocbptr)malloc(sizeof(struct aiocb));

    memset(server_aiocb, 0, sizeof(struct aiocb));

    int BUFSIZE = 0;
    unsigned int sof_int = sizeof(BUFSIZE);

    getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&BUFSIZE, &sof_int);

    server_aiocb->aio_buf = malloc(BUFSIZE + 1);
    server_aiocb->aio_fildes = fd;
    server_aiocb->aio_nbytes = BUFSIZE;
    server_aiocb->aio_offset = 0;

    while (aio_error(server_aiocb) == EINPROGRESS);

    return server_aiocb;
}

ssize_t async_oper(int fd, char* buffer, size_t count, async_operation operation) {
    aiocbptr cbp = generate_ioreq(fd);

    cbp->aio_buf = malloc(count);
    memcpy((void*)cbp->aio_buf, buffer, count);

    cbp->aio_nbytes = count;

    int retval = 0;

    switch (operation) {
    case ASYNC_READ:
        printf("commencing with aio_read\n");
        retval = aio_read(cbp);
        break;
    case ASYNC_WRITE:
        printf("commencing with aio_write\n");
        retval = aio_write(cbp);
        break;
    }

    while (retval == EINPROGRESS);

    ssize_t result = aio_return(cbp);
    return  result;
}

void free_ioreqs() {
    free(ioreqs);
}
