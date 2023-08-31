#ifndef ASYNC_H
#define ASYNC_H

#include <aio.h>

struct aiocb* new_aio(int fd);
struct aiocb* generate_ioreq(int fd);
ssize_t       async_read(int fd, char* buffer, size_t count);
ssize_t       async_write(int fd, char* buffer, size_t count);
void          free_ioreqs();

extern struct aiocb* ioreqs;
extern size_t        numreqs;
extern size_t        inxreqs;

#endif
