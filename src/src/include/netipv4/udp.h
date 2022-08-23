/*
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This is the UDP include file.
 *
 */
#include <sys/types.h>
#include <sys/network.h>
#include <net/socket.h>

#ifndef __UDP_H__
#define __UDP_H__

/* based on RFC 792 */
struct UDP_HEADER {
	uint8_t		source[2];
	uint8_t		dest[2];
	uint8_t		length[2];
	uint8_t		cksum[2];
} __attribute__((packed));

int udp_handle_packet (struct NETPACKET* np);
int udp_xmit_packet (struct SOCKET* s, uint32_t dest, uint16_t port, uint8_t* data, uint32_t len);
int udp_xmit_packet_ex (struct DEVICE* dev, uint8_t* hwaddr, uint32_t source, uint32_t dest, uint16_t srcport, uint16_t port, uint8_t* data, uint32_t len);

#endif /* __UDP_H__ */
