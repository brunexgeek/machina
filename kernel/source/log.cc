#include <sys/log.h>
#include <mc/stdio.h>

int klog_print(const char *format, ...)
{
    va_list va;
    va_start(va, format);
    int result = vprintf(format, va);
    va_end(va);
    return result;
}