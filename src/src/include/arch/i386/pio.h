/*
 * pio.h - XeOS i386 PIO Code
 * (c) 2002, 2003 Rink Springer, BSD
 *
 * This include file 
 *
 */
#include <sys/types.h>

#ifndef __PIO_H__
#define __PIO_H__

inline void outb (uint16_t port, uint8_t data);
inline void outw (uint16_t port, uint16_t data);
inline void outd (uint16_t port, uint32_t data);

inline uint8_t inb (uint16_t port);
inline uint16_t inw (uint16_t port);
inline uint32_t ind (uint16_t port);

inline void insb (uint16_t port, void* addr, size_t cnt);
inline void insw (uint16_t port, void* addr, size_t cnt);
inline void insd (uint16_t port, void* addr, size_t cnt);

inline void outsb (uint16_t port, void* addr, size_t cnt);
inline void outsw (uint16_t port, void* addr, size_t cnt);
inline void outsd (uint16_t port, void* addr, size_t cnt);

#endif /* __PIO_H__ */
