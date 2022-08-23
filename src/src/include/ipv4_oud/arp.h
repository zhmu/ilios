/*
 * arp.h - ILIOS IPv4 ARP
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 */
#include <sys/types.h>

#ifndef __ARP_H__
#define __ARP_H__

#define ARP_CACHE_SIZE		128
#define ARP_TIMEOUT		600

#define ARP_FLAG_PERM		1

/* based on RFC 826 */
typedef struct {
	uint16_t		hw_type;
	uint16_t		protocol;
	uint8_t			hw_len;
	uint8_t			proto_len;
	uint16_t		opcode;
	uint8_t			hw_source[6];
	uint8_t			source_addr[4];
	uint8_t			hw_dest[6];
	uint8_t			dest_addr[4];
} ARP_PACKET __attribute__((packed));

#define ARP_HWTYPE_ETH		1

#define ARP_REQUEST		1
#define ARP_REPLY		2

typedef struct {
	uint32_t		address;
	uint8_t			hw_addr[6];
	uint32_t		timestamp;
	uint8_t			flags;
	struct DEVICE*		device;
} ARP_RECORD;

extern ARP_RECORD* arp_cache;

void arp_init();
void arp_tick();
void arp_request (struct DEVICE* dev, uint32_t source, uint32_t addr);

void arp_add_cache (struct DEVICE* dev, uint32_t addr, uint8_t* hw_addr, uint8_t flags);
void arp_remove_cache (uint32_t addr);
void arp_flush();

int arp_get_address (uint32_t addr, uint8_t* hw_addr);

#endif /* __ARP_H__ */
