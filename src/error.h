#ifndef ERROR_H
#define ERROR_H

#define EOK         0
#define EIO         1
#define EINVARG     2
#define ENOMEM      3
#define EBADPATH    4
#define EINVALID    5
#define EREADONLY   6
#define ETAKEN      7

#define ISERR(v)    ((v) < 0)
#define ERRTOPTR(e) ((void *)(e))
#define PTRTOERR(p) ((int)(p))

extern int errno;

#endif