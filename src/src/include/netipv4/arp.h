/*
 *
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This is the ARP include file, by RFC 826.
 *
 */
#ifndef __ARP_H__
#define __ARP_H__

#include <sys/types.h>
#include <sys/device.h>

#define ARP_FLAGS_PERMANENT 1

#define ARP_HWTYPE_ETH		1

#define ARP_REQUEST 0x01
#define ARP_REPLY   0x02

struct ARP_PACKET {
	uint8_t	hw_type[2];
	uint8_t	protocol[2];
	uint8_t	hw_len;
	uint8_t proto_len;
	uint8_t	opcode[2];
	uint8_t hw_source[ETHER_ADDR_LEN];	
	uint8_t source_addr[4];
	uint8_t hw_dest[ETHER_ADDR_LEN];	
	uint8_t dest_addr[4];
} __attribute__((packed));

struct ARP_RECORD {
	uint32_t       address;
	uint8_t        hw_addr[ETHER_ADDR_LEN];
	uint32_t       timestamp;
	uint8_t        flags;
	struct DEVICE* device;
};

extern struct ARP_RECORD* arp_cache;

void arp_init();
int arp_handle_packet (struct NETPACKET* pkt);
void arp_flush();
void arp_flush_device(struct DEVICE* dev);

int arp_add_record (uint32_t h, char* hw, struct DEVICE* dev, uint8_t fl);
int arp_remove_record (uint32_t h, struct DEVICE* dev);
struct ARP_RECORD* arp_find_record (uint32_t h);
int arp_update_record (uint32_t h, char* hw, struct DEVICE* dev);

int arp_send_request (uint32_t addr, struct DEVICE* fdev);
struct ARP_RECORD* arp_fetch_address (uint32_t addr);

#endif /* __ARP_H__ */
