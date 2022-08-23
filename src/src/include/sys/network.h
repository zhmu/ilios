/*
 * network.h - XeOS Network Stuff
 * (c) 2003 Rink Springer, BSD
 *
 * This include file describes some stuff for networking.
 *
 */
#include <sys/types.h>
#include <config.h>

#ifndef __NETWORK_H__
#define __NETWORK_H__

#define NETWORK_MAX_PACKET_LEN	2048
#define NETWORK_MTU             NETWORK_MAX_PACKET_LEN
#define NETWORK_TXBUFFER_SIZE		64
#define ETHER_ADDR_LEN					6

#define ETHERTYPE_IP            0x0800
#define ETHERTYPE_ARP           0x0806

#define NETPACKET_TYPE_RECV			0
#define NETPACKET_TYPE_XMIT			0x80

typedef struct  __attribute__((__packed__)) {
	uint8_t			dest[ETHER_ADDR_LEN];
	uint8_t			source[ETHER_ADDR_LEN];
	uint8_t			type[2];
} ETHERNET_HEADER;

typedef struct {
	uint8_t			hw_addr[ETHER_ADDR_LEN];
} ETHERNET_OPTIONS;

struct NETPACKET {
	struct NETPACKET* next;

	struct DEVICE*	device;
	uint8_t	type;
	size_t len;
	size_t header_len;
	char	 frame[NETWORK_MAX_PACKET_LEN] __attribute__((aligned(16)));
	char*	 data;
};

extern struct NETPACKET* network_netpacket;
extern struct NETPACKET* netpacket_first_avail;
extern struct NETPACKET* netpacket_todo_first;
extern struct NETPACKET* netpacket_todo_last;
extern int network_numbuffers;

#ifdef __KERNEL
void network_init();
struct NETPACKET* network_alloc_packet (struct DEVICE* dev);
void network_free_packet (struct NETPACKET* pkt);

void network_queue_packet (struct NETPACKET* pkt);
void network_handle_queue();
void network_xmit_frame (struct DEVICE* dev, struct NETPACKET* nb);
void network_xmit_packet (struct DEVICE* dev, struct NETPACKET* pkt, void* addr);
struct NETPACKET* network_get_next_txbuf (struct DEVICE* dev);
#endif /* __KERNEL */

#endif /* __NETWORK_H__ */

/* vim:set ts=2 sw=2: */
