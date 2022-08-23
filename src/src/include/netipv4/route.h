/*
 *
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This will handle routing.
 *
 */
#include <sys/types.h>
#include <sys/device.h>

#ifndef __ROUTE_H__
#define __ROUTE_H__

/* ROUTE_MAX_ENTRIES defines the number of entries in the routing table */
#define ROUTE_MAX_ENTRIES	1024

/* ROUTE_FLAG_xxx defines various routing entry flags */
#define ROUTE_FLAG_INUSE	1
#define ROUTE_FLAG_PERM		2
#define ROUTE_FLAG_GATEWAY	4

struct ROUTE_ENTRY {
	struct DEVICE*	device;
	uint32_t	network;
	uint32_t	mask;
	uint32_t	gateway;
	uint32_t	flags;
};

void route_init();
int route_add (struct DEVICE* dev, uint32_t dest, uint32_t mask, uint32_t gateway, uint32_t flags);
int route_remove (uint32_t dest, uint32_t mask);
struct IPV4_ADDR* route_find_ip (struct DEVICE* dev, uint32_t dest);
void route_flush();

struct DEVICE* route_find_device (uint32_t dest);

extern struct ROUTE_ENTRY* routes;

#endif /* __ROUTE_H__ */
