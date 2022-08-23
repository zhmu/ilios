/*
 * util.h - ILIOS Utility Stuff
 * (c) 2002, 2003 Rink Springer, BSD
 *
 * This file contains helper stuff for main/
 *
 */
#include <sys/device.h>

#ifndef __UTIL_H__
#define __UTIL_H__

struct DRIVER {
	char* name;
	int (*init)(char* name, struct DEVICE_RESOURCES* cf);
};

int ip_fetch_addr (char** buf, uint32_t* addr);
struct DRIVER* find_driver (char* name);

#endif /* __UTIL_H__ */
