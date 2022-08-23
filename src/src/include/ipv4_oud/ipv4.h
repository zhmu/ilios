/*
 * ipv4.h - ILIOS IPv4 stack
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 */
#include <sys/types.h>

#ifndef __IPV4_H__
#define __IPV4_H__

#define IPV4_MAX_ADDR 16

/* based on RFC 1700 */
#define IPHDR_PROTO_ICMP	1
#define IPHDR_PROTO_IP		4

typedef struct {
	uint32_t    addr;
	uint32_t		netmask;
} IPV4_ADDR;

/* IPv4 configuration options */
struct IPV4_CONFIG {
	IPV4_ADDR	  address[IPV4_MAX_ADDR];
};

#define ETHERTYPE_IP	0x0800
#define ETHERTYPE_ARP	0x0806

/* debugging */
#define xARP_DEBUG
#define xIP_VERBOSE
#define xICMP_VERBOSE

struct DEVICE;

void ipv4_init();
void ipv4_tick();

int ipv4_isbound (struct DEVICE* dev, uint32_t addr);
int ipv4_add_address (struct DEVICE* dev, uint32_t addr, uint32_t netmask);
int ipv4_remove_address (struct DEVICE* dev, uint32_t addr);
int ipv4_is_broadcast (uint32_t addr);
int ipv4_is_ours (uint32_t addr);

uint32_t ipv4_conv_addr (uint8_t* addr);

#endif /* __IPV4_H__ */

/* vim:set ts=2 sw=2: */
