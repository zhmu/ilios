/*
 * string.c - XeOS string stuff.
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 * This code will handle the string stuff.
 *
 */
#include <sys/types.h>
#include <sys/kmalloc.h>
#include <stdarg.h>
#include <lib/lib.h>

#if ARCH!=i386
/*
 * kmemcpy (void* dst, void* src, size_t len)
 *
 * This will copy [len] bytes from [src] to [dest].
 *
 */
void
kmemcpy (void* dst, const void* src, size_t len) {
#if 1
	register const uint8_t* sptr = (const uint8_t*)src;
	register uint8_t* dptr = (uint8_t*)dst;
	while (len--) *dptr++ = *sptr++;
#else
	asm ("rep movsb"
			 :
			 : "S" (src), "D" (dst), "c" (len));
#endif
}

/*
 * kmemset (void* b, const char c, size_t len)
 *
 * This will fill [len] bytes of [b] with [c].
 *
 */
void
kmemset (void* b, const char c, size_t len) {
#if 0 
	asm ("rep stosb"
			:
			: "S" (b), "a" (c), "c" (len));
#else
	register uint8_t* ptr = (uint8_t*)b;
	while (len--) *ptr++ = c;
#endif
}

/*
 * kstrlen (const char* s)
 *
 * This will return the length of zero-terminated string [s].
 *
 */
size_t
kstrlen (const char* s) {
	size_t len = 0;
	if (s == NULL)
		return 0;

	while (*s++) len++;
	return len;
}

/*
 * kstrcmp (const char* s1, const char* s2)
 *
 * This will return zero if [s1] and [s2] are equal, or 1 if they are not.
 *
 */
int
kstrcmp (const char* s1, const char* s2) {
	while (*s1)
		if (*s1++ != *s2++)
			return 1;
	return (*s1 == *s2) ? 0 : 1;
}

/*
 * kstrncmp (const char* s1, const char* s2, size_t len)
 *
 * This will compare [s1] and [s2], up to [len] chars.
 *
 */
int
kstrncmp (const char* s1, const char* s2, size_t len) {
	if (!len)
		return 0;
	while (--len)
		if (*s1++ != *s2++)
			return 1;
	return (*s1 == *s2) ? 0 : 1;
}

/*
 * kmemcmp (const char* s1, const char* s2, size_t len)
 *
 * This will return zero if [s1] and [s2] are equal, or 1 if they are not.
 *
 */
int
kmemcmp (const char* s1, const char* s2, size_t len) {
	if (!len)
		return 0;

	while (len--)
		/* match? */
		if (*s1 != *s2)
			/* no. too bad */
			return 1;

	/* match */
	return 0;
}

/*
 * kstrchr (const char* s1, char ch)
 *
 * This will search for [ch] in [s1]. It will return a pointer to [ch] in [s1]
 * on success or NULL if it cannot be found.
 *
 */
char*
kstrchr (const char* s1, char ch) {
	while (1) {
		/* match? */
		if (*s1 == ch)
			/* yes. return it */
			return (char*)s1;

		/* null? */
		if (!*s1)
			/* yes. bail out */
			break;

		/* next */
		s1++;
	}

	/* no match */
	return NULL;
}

/*
 * kstrcpy (char* dst, const char* src)
 *
 * This will copy [src] to [dst].
 *
 */
int
kstrcpy (char* dst, const char* src) {
	/* copy the chars */
	while (*src)
		*dst++ = *src++;

	/* copy trailing NUL too! */
	*dst = *src;
	return 1;
}
#endif
/*
 * kstrdup (const char* s)
 *
 * This will duplicate string [s] in a freshly kmalloc()-ed piece of memory. The
 * result will be returned.
 *
 */
char*
kstrdup (const char* s) {
	char* ptr;

	if (s == NULL)
		return NULL;

	ptr = (char*)kmalloc (NULL, kstrlen (s) + 1, 0);
	kmemcpy (ptr, s, kstrlen (s) + 1);
	return ptr;
}


/* vim:set ts=2 sw=2: */
