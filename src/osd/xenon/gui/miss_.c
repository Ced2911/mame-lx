#include <time/time.h>
#include <sys/reent.h>
#include <ppc/atomic.h>
#include <assert.h>
#include <xenon_uart/xenon_uart.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <time/time.h>
#include <xenon_smc/xenon_smc.h>

// miss in gligli libxenon
int stat(const char * __restrict path, struct stat * __restrict buf) {
    int fd = -1;
    fd = open(path, O_RDONLY);

    if (fd) {
        return fstat(fd, buf);
    }
    return ENOENT; // file doesn't exist
}
