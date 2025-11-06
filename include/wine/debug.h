#ifndef __WIBO_WINE_DEBUG_H
#define __WIBO_WINE_DEBUG_H

/* wibo: minimal Wine debug stubs. */

#define WINE_DEFAULT_DEBUG_CHANNEL(name)

#define TRACE(...)   ((void)0)
#define WARN(...)    ((void)0)
#define ERR(...)     ((void)0)

#define TRACE_ON(channel) (0)
#define FIXME(...)  ((void)0)

#include <stdarg.h>
#include <stdio.h>

static inline const char *wine_dbg_sprintf(const char *fmt, ...)
{
    static char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return buf;
}

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

static inline const char *debugstr_a(const char *str)
{
    return str ? str : "<null>";
}

static inline const wchar_t *debugstr_w(const wchar_t *str)
{
    return str ? str : L"<null>";
}

#endif
