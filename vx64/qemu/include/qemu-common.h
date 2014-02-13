
/* Common header file that is included by all of QEMU.
 *
 * This file is supposed to be included only by .c files. No header file should
 * depend on qemu-common.h, as this would easily lead to circular header
 * dependencies.
 *
 * If a header file uses a definition from qemu-common.h, that definition
 * must be moved to a separate header file, and the header that uses it
 * must include that header.
 */
#ifndef QEMU_COMMON_H
#define QEMU_COMMON_H

#include "qemu/compiler.h"
#include "config-host.h"
#include "qemu/typedefs.h"

#if defined(__arm__) || defined(__sparc__) || defined(__mips__) || defined(__hppa__) || defined(__ia64__)
#define WORDS_ALIGNED
#endif

#define TFR(expr) do { if ((expr) != -1) break; } while (errno == EINTR)

/* we put basic includes here to avoid repeating them in device drivers */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <assert.h>
#include <signal.h>

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif
#ifndef ENOMEDIUM
#define ENOMEDIUM ENODEV
#endif
#if !defined(ENOTSUP)
#define ENOTSUP 4096
#endif
#if !defined(ECANCELED)
#define ECANCELED 4097
#endif
#if !defined(EMEDIUMTYPE)
#define EMEDIUMTYPE 4098
#endif
#ifndef TIME_MAX
#define TIME_MAX LONG_MAX
#endif

/* HOST_LONG_BITS is the size of a native pointer in bits. */
#if UINTPTR_MAX == UINT32_MAX
# define HOST_LONG_BITS 32
#elif UINTPTR_MAX == UINT64_MAX
# define HOST_LONG_BITS 64
#else
# error Unknown pointer size
#endif

typedef int (*fprintf_function)(FILE *f, const char *fmt, ...)
    GCC_FMT_ATTR(2, 3);

extern int use_icount;

#include "cpu.h"

/* cutils.c */
void pstrcpy(char *buf, int buf_size, const char *str);
void strpadcpy(char *buf, int buf_size, const char *str, char pad);
char *pstrcat(char *buf, int buf_size, const char *s);
int strstart(const char *str, const char *val, const char **ptr);
int stristart(const char *str, const char *val, const char **ptr);
int qemu_strnlen(const char *s, int max_len);
time_t mktimegm(struct tm *tm);


int parse_uint(const char *s, unsigned long long *value, char **endptr,
               int base);
int parse_uint_full(const char *s, unsigned long long *value, int base);

/*
 * strtosz() suffixes used to specify the default treatment of an
 * argument passed to strtosz() without an explicit suffix.
 * These should be defined using upper case characters in the range
 * A-Z, as strtosz() will use qemu_toupper() on the given argument
 * prior to comparison.
 */
#define STRTOSZ_DEFSUFFIX_TB	'T'
#define STRTOSZ_DEFSUFFIX_GB	'G'
#define STRTOSZ_DEFSUFFIX_MB	'M'
#define STRTOSZ_DEFSUFFIX_KB	'K'
#define STRTOSZ_DEFSUFFIX_B	'B'
int64_t strtosz(const char *nptr, char **end);
int64_t strtosz_suffix(const char *nptr, char **end, const char default_suffix);
int64_t strtosz_suffix_unit(const char *nptr, char **end,
                            const char default_suffix, int64_t unit);

/* path.c */
void init_paths(const char *prefix);
const char *path(const char *pathname);

#define qemu_isalnum(c)		isalnum((unsigned char)(c))
#define qemu_isalpha(c)		isalpha((unsigned char)(c))
#define qemu_iscntrl(c)		iscntrl((unsigned char)(c))
#define qemu_isdigit(c)		isdigit((unsigned char)(c))
#define qemu_isgraph(c)		isgraph((unsigned char)(c))
#define qemu_islower(c)		islower((unsigned char)(c))
#define qemu_isprint(c)		isprint((unsigned char)(c))
#define qemu_ispunct(c)		ispunct((unsigned char)(c))
#define qemu_isspace(c)		isspace((unsigned char)(c))
#define qemu_isupper(c)		isupper((unsigned char)(c))
#define qemu_isxdigit(c)	isxdigit((unsigned char)(c))
#define qemu_tolower(c)		tolower((unsigned char)(c))
#define qemu_toupper(c)		toupper((unsigned char)(c))
#define qemu_isascii(c)		isascii((unsigned char)(c))
#define qemu_toascii(c)		toascii((unsigned char)(c))

void *qemu_oom_check(void *ptr);

/* Error handling.  */

void QEMU_NORETURN hw_error(const char *fmt, ...) GCC_FMT_ATTR(1, 2);


void tcg_exec_init(unsigned long tb_size);
bool tcg_enabled(void);

void cpu_exec_init_all(void);

/* Convert a byte between binary and BCD.  */
static inline uint8_t to_bcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

static inline uint8_t from_bcd(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0f);
}

/* compute with 96 bit intermediate result: (a*b)/c */
static inline uint64_t muldiv64(uint64_t a, uint32_t b, uint32_t c)
{
    union {
        uint64_t ll;
        struct {
#ifdef HOST_WORDS_BIGENDIAN
            uint32_t high, low;
#else
            uint32_t low, high;
#endif
        } l;
    } u, res;
    uint64_t rl, rh;

    u.ll = a;
    rl = (uint64_t)u.l.low * (uint64_t)b;
    rh = (uint64_t)u.l.high * (uint64_t)b;
    rh += (rl >> 32);
    res.l.high = rh / c;
    res.l.low = (((rh % c) << 32) + (rl & 0xffffffff)) / c;
    return res.ll;
}

/* Round number down to multiple */
#define QEMU_ALIGN_DOWN(n, m) ((n) / (m) * (m))

/* Round number up to multiple */
#define QEMU_ALIGN_UP(n, m) QEMU_ALIGN_DOWN((n) + (m) - 1, (m))

static inline bool is_power_of_2(uint64_t value)
{
    if (!value) {
        return 0;
    }

    return !(value & (value - 1));
}

/* round down to the nearest power of 2*/
int64_t pow2floor(int64_t value);

#include "qemu/module.h"

/*
 * Implementation of ULEB128 (http://en.wikipedia.org/wiki/LEB128)
 * Input is limited to 14-bit numbers
 */

int uleb128_encode_small(uint8_t *out, uint32_t n);
int uleb128_decode_small(const uint8_t *in, uint32_t *n);

#endif
