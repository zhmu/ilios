/*
 * startup.c - XeOS i386 Startup code
 * (c) 2002 Rink Springer, BSD licensed
 *
 * This code is heavily based on YCX2 (src/ycx2/arch/i386/jumpmain.c), (c) 2001,
 * 2002 Anders Gavare.
 *
 * This code will create a paging table, so references are not messed up.
 *
 */
#include <sys/types.h>
#include <md/config.h>
#include <md/memory.h>

/* forward declaration to kernel main */
void kmain();

/* our own stuff */
size_t	highest_addressable_byte = 0;
size_t	first_addressable_high_byte = 0;
size_t	lowest_addressable_free_byte = 0;

/*
 * This will scan [ptr] for 0xff bytes, to probe whether memory exists or not.
 * It will return 0 if [ptr] only has 0xff bytes, or 1 if it does not.
 */
int
i386_trymemory (uint8_t* ptr, size_t len) {
	size_t i;

	/* scan the range */
	for (i = 0; i < len; i++)
		/* 0xff byte here? */
		if (ptr[i] != 0xff)
			return 1;

	/* only 0xff bytes here */
	return 0;
}

/*
 * This returns the size of the kernel.
 */
size_t
i386_getkernelsize() {
	/* FIXME: how to calculate ELF sizes ??? */
	return 300000;
}

/*
 * This will return where the kernel is loaded.
 *
 * FIXME: For now, this function only guesses...
 */
size_t i386_getkernelloadaddr() {
	int addr;

	addr = (int) &i386_getkernelloadaddr;
	addr &= 0x00ffffff;

	if (addr >= 0x100000)
		return 0x100000;
	else
		return 0x10000;
}

/*
 * This is the very evil main code, as directly called by the assembly stub.
 */
void
__main() {
	size_t highest, try;
	ssize_t trylen;
	size_t top_of_memory, bottom_of_high_memory;
	int kernel_image_size;
	size_t low_memory_usage = 0;
	size_t high_memory_usage = 0;

	/* now, find the last byte of memory we can use */
	try = i386_getkernelloadaddr() + i386_getkernelsize();
	trylen = 0x100000;      /*  step size  */
	highest = try;

	/*
	 * since our memory allocator only knows pages, we try the very first byte
	 * of every page.   if we can write 01010101 and then 10101010 to it and
	 * successfully read it back, we can assume the memory there exists.
	 */
	while (1) {
		*(uint8_t*)try = 0x55;
		if (*(uint8_t*)try != 0x55) break;
		*(uint8_t*)try = 0xaa;
		if (*(uint8_t*)try != 0xaa) break;

		try += (PAGESIZE);
	}
	highest = try - PAGESIZE;
	top_of_memory = highest;

	/* calculate the number of pagetables we need. each pagetable covers 4MB,
	 * so it's just division really */
	highest |= 0x3fffff;

	/*  Estimate the size of the kernel image in memory:  */
	kernel_image_size = i386_getkernelsize();
	kernel_image_size = ((kernel_image_size - 1) | (PAGESIZE-1)) + 1;

	if (i386_getkernelloadaddr() < 1048576)
		low_memory_usage += kernel_image_size;
	else
		high_memory_usage += kernel_image_size;

	bottom_of_high_memory = 0x100000 + high_memory_usage;

	highest_addressable_byte = top_of_memory;
	first_addressable_high_byte = bottom_of_high_memory;

	/*  0x10000 = above initial stack  */
	lowest_addressable_free_byte = 0x10000 + low_memory_usage;

	/* go to the main code */
	kmain();
}

/* vim:set sw=2 ts=2: */
