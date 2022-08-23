/*
 *
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This is the main include file.
 *
 */
#include <sys/types.h>
#include <sys/network.h>

#ifndef __INET4_H__
#define __INET4_H__

/* ARP_CACHE_SIZE is the size of our ARP cache, in records */
#define ARP_CACHE_SIZE 128

/* IPV4_MAX_ADDR is the number of IPv4 addresses a single NIC can have */
#define IPV4_MAX_ADDR 16

struct IPV4_ADDR {
	uint32_t	addr;
	uint32_t	netmask;
};

struct IPV4_CONFIG {
	struct IPV4_ADDR address[IPV4_MAX_ADDR];
};

extern int ipv4_routing;

int ipv4_handle_packet (struct NETPACKET* pkt);
uint32_t ipv4_conv_addr (uint8_t* addr);

struct IPV4_ADDR* ipv4_is_device_bound (uint32_t addr, struct DEVICE* dev);
struct DEVICE* ipv4_is_bound (uint32_t addr);
int ipv4_is_valid (uint32_t addr, struct IPV4_ADDR* ap);

void ipv4_init();
void ipv4_tick();
int ipv4_add_address (struct DEVICE* dev, uint32_t addr, uint32_t mask);
int ipv4_remove_address (struct DEVICE* dev, uint32_t addr);
void ipv4_purge_device (struct DEVICE* dev);
uint32_t ipv4_guess_netmask (uint32_t addr);
void ipv4_set_routing (int val);

#endif /* __INET4_H__ */
