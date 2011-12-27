#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <debug.h>
#include <stdarg.h>

int getrusage(int who, struct rusage *r_usage) {
    return 0;
}

void mame_printf_verbose(const char *text, ...){
    TR;
    char buffer[256];
    va_list args;
    va_start (args, text);
    vsprintf (buffer,text, args);
    printf(buffer);
    va_end (args);
}