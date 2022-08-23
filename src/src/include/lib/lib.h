/*
 * lib.h - XeOS Kernelspace Library Include File
 * (c) 2002, 2003 Rink Springer, BSD
 *
 * This include file has the kernelspace library declarations.
 *
 */

#ifdef __KERNEL

#include <sys/types.h>
#include <stdarg.h>

#ifndef __KLIB_H__
#define __KLIB_H__

/* string.c */
void kmemcpy (void* dst, const void* src, size_t len);
void kmemset (void* dst, const char c, size_t len);
int  kmemcmp (const char* s1, const char* s2, size_t len);
size_t kstrlen (const char* s);
char* kstrdup (const char* s);
int kstrcmp (const char* s1, const char* s2);
int kstrncmp (const char* s1, const char* s2, size_t len);
char* kstrchr (const char* s1, char ch);
int kstrcpy (char* dst, const char* src);

/* kprint.c */
void vaprintf (char* fmt, va_list ap);
void kprintf (char* fmt, ...);

/* panic.c */
void panic (char* fmt, ...);

/* input.c */
const char* gets();
long int strtol (const char *nptr, char **endptr, int base);

/* *.S */
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

#endif /* __KLIB_H__ */

#endif /* __KERNEL */
