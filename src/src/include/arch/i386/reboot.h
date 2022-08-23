/*
 * reboot.h - XeOS Machine Dependant Reboot Stuff
 * (c) 2003 Rink Springer, BSD
 *
 * This include file is for the machine dependant reboot stuff.
 *
 */
#include <sys/types.h>

#ifndef __MD_REBOOT_H__
#define __MD_REBOOT_H__

#ifdef __KERNEL
void arch_reboot();
void arch_relax();
#endif /* __KERNEL */

#endif /* __MD_REBOOT_H__ */
