#ifndef ASYNC_H
#define ASYNC_H

#include <aio.h>

// implement these #define's correctly for use later
#define async_read(a, b, c) async_oper(a, b, c, ASYNC_READ)
#define async_write(a, b, c) async_oper(a, b, c, ASYNC_WRITE)

typedef enum
{
    ASYNC_READ,
    ASYNC_WRITE,
} async_operation;

typedef struct aiocb* aiocbptr;

aiocbptr  new_aio        (int fd);
aiocbptr  generate_ioreq (int fd);
aiocbptr  init_ioreqs    ();
aiocbptr  add_ioreqsq    ();
ssize_t   async_oper     (int fd, char* buffer, size_t count, async_operation operation);
void      free_ioreqs    ();

extern struct aiocb* ioreqs;
extern size_t        numreqs;
extern size_t        inxreqs;

#endif
