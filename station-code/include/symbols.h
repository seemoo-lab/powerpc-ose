
#ifndef SYM_SYMBOLS_H
#define SYM_SYMBOLS_H
// generate `address.h` from ELF using `./genAddressInclude.sh ../tbrc_ram.elf`
#include "./address.h"
#include "./types.h"
#include <stdarg.h>

// use static inline functions whenever possible. not possible when dealing with variable arguments.

#define printf(format...) ((void(*)(const char * , ...))SYM_ADDR_printf)(format);
#define sscanf(s, format...) ((void(*)(const char * , ...))SYM_ADDR_sscanf)(s, format);

// E.g., assuming there is a function sleep at address SYM_ADDR_sleep:
//static inline void sleep(uint32_t sec) { ((void(*)())SYM_ADDR_sleep)(sec); }


/**
 * Memory
 */
static inline void* malloc(uint32_t size) { return ((void*(*)())SYM_ADDR_malloc)(size); }
static inline void free( void* ptr) { ((void*(*)())SYM_ADDR_free)(ptr); }
static inline void *memcpy (void *destination, const void *source, uint32_t num) { return ((void*(*)(void*, const void*, uint32_t))SYM_ADDR_memcpy)(destination, source, num); }

static inline int memcmp(const void *str1, const void *str2, uint32_t n) { return ((int(*)(const void*, const void*, uint32_t))SYM_ADDR_memcmp)(str1, str2, n); }
static inline void *memset(void *s, int c, uint32_t n) { return ((void*(*)(void*, int, uint32_t))SYM_ADDR_memset)(s, c, n); }


/**
 * Strings
 */
static inline char *strcpy (char *destination, const char *source ) { return ((char*(*)(void*, const void*))SYM_ADDR_strcpy)(destination, source); }
static inline char *strncpy (char *destination, const char *source, uint32_t size ) { return ((char*(*)(void*, const void*, uint32_t))SYM_ADDR_strncpy)(destination, source, size); }
static inline uint32_t strlen (const char *str) { return ((uint32_t(*)(const void*))SYM_ADDR_strlen)(str); }
static inline char *strncat (char *destination, const char *source, uint32_t n) { return ((char*(*)(void*, const void*, uint32_t))SYM_ADDR_strncat)(destination, source, n); }
static inline int vsprintf (char *s, const char * format, ...) {
    va_list args;
    va_start (args, format);
    int ret = ((int(*)(char*, const char*, va_list))SYM_ADDR_vsprintf)(s, format, args);
    va_end (args);
    return ret;
}
static inline int vsnprintf (char *s, uint32_t n, const char * format, ...) {
    va_list args;
    va_start (args, format);
    int ret = ((int(*)(char*, uint32_t, const char*, va_list))SYM_ADDR_vsnprintf)(s, n, format, args);
    va_end (args);
    return ret;
}


/**
 * I/O
 */
static inline uint32_t* fopen(const char *filename, const char *mode) { return ((uint32_t*(*)(const char*, const char*))SYM_ADDR_fopen)(filename, mode); }
static inline uint32_t fclose (uint32_t *fp) { return ((uint32_t(*)(uint32_t*))SYM_ADDR_fclose)(fp); }
static inline uint32_t fread(void *ptr, uint32_t size, uint32_t nitems, uint32_t *fp) {
    return ((uint32_t(*)(uint32_t*, uint32_t, uint32_t, uint32_t*))SYM_ADDR_fread)(ptr, size, nitems, fp);
}
static inline uint32_t fwrite(void *ptr, uint32_t size, uint32_t nitems, uint32_t *fp) {
    return ((uint32_t(*)(uint32_t*, uint32_t, uint32_t, uint32_t*))SYM_ADDR_fwrite)(ptr, size, nitems, fp);
}



/**
 * Random number generator
 */

static inline uint32_t rand() { return ((uint32_t(*)(void))SYM_ADDR_rand)(); }
static inline void srand(uint32_t seed) { ((void(*)(uint32_t))SYM_ADDR_srand)(seed); }


/**
 * Type conversion
 */

static inline int atoi(const char *str) { return ((int(*)(const char*)) SYM_ADDR_atoi)(str); }

#endif

