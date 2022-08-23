/*
 * route.h - ILIOS IPv4 routing
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 */
#include <sys/types.h>
#include <sys/device.h>

#ifndef __ROUTE_H__
#define __ROUTE_H__

#define ROUTE_MAX_ENTRIES	1024

#define ROUTE_FLAG_INUSE	1
#define ROUTE_FLAG_PERM		2
#define ROUTE_FLAG_GATEWAY	4

typedef struct {
	struct DEVICE*	device;
	uint32_t	network;
	uint32_t	mask;
	uint32_t	gateway;
	uint32_t	flags;
} ROUTE_ENTRY;

void route_init();
int route_add (struct DEVICE* dev, uint32_t dest, uint32_t mask, uint32_t gateway, uint32_t flags);
int route_remove (uint32_t dest, uint32_t mask);
int route_check_dup (uint32_t dest, uint32_t netmask);
uint32_t route_find_ip (struct DEVICE* dev, uint32_t dest);
void route_flush();

struct DEVICE* route_find_device (uint32_t dest);

extern ROUTE_ENTRY* routes;

#endif /* __ROUTE_H__ */
