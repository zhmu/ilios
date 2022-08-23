/*
 * pio.c - XeOS i386 IO code
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 * This code will handle reading and writing to I/O ports
 *
 */
#include <sys/types.h>
#include <md/pio.h>

inline void
outb (uint16_t port, uint8_t data) {
	asm ("outb %0,%1"
	:
	: "a" (data), "id" (port));
}

inline void
outw (uint16_t port, uint16_t data) {
	asm ("outw %0,%1"
	:
	: "a" (data), "d" (port));
}

inline void
outd (uint16_t port, uint32_t data) {
	asm ("outl %0,%1"
	:
	: "a" (data), "d" (port));
}

inline uint8_t
inb (uint16_t port) {
	uint8_t data;

	asm ("inb %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

inline uint16_t
inw (uint16_t port) {
	uint16_t data;

	asm ("inw %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

inline uint32_t
ind (uint16_t port) {
	uint32_t data;

	asm ("inl %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

inline void
insb (uint16_t port, void* addr, size_t cnt) {
   asm ("cld\nrepne\ninsb" :
	"=D" (addr), "=c" (cnt) :
	"d" (port), "0" (addr), "1" (cnt) :
	"memory");
}

inline void
insw (uint16_t port, void* addr, size_t cnt) {
   asm ("cld\nrepne\ninsw" :
	"=D" (addr), "=c" (cnt) :
	"d" (port), "0" (addr), "1" (cnt) :
	"memory");
}

inline void
insd (uint16_t port, void* addr, size_t cnt) {
   asm ("cld\nrepne\ninsl" :
	"=D" (addr), "=c" (cnt) :
	"d" (port), "0" (addr), "1" (cnt) :
	"memory");
}

inline void
outsb (uint16_t port, void* addr, size_t cnt) {
   asm ("cld\nrepne\noutsb" :
	"=S" (addr), "=c" (cnt) :
	"d" (port), "0" (addr), "1" (cnt) :
	"memory");
}

inline void
outsw (uint16_t port, void* addr, size_t cnt) {
   asm ("cld\nrepne\noutsw" :
	"=S" (addr), "=c" (cnt) :
	"d" (port), "0" (addr), "1" (cnt) :
	"memory");
}

inline void
outsd (uint16_t port, void* addr, size_t cnt) {
   asm ("cld\nrepne\noutsl" :
	"=S" (addr), "=c" (cnt) :
	"d" (port), "0" (addr), "1" (cnt) :
	"memory");
}

/* vim:set ts=2 sw=2: */
