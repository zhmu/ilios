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

#ifndef __IP_H__
#define __IP_H__

/* based on RFC 1700 */
#define IP_PROTO_ICMP   1
#define IP_PROTO_IP     4

/* based on RFC 790 */
#define IP_PROTO_TCP	6
#define IP_PROTO_UDP	17

/* based on RFC 791 */
struct IP_HEADER {
	uint8_t			version_ihl;
	uint8_t			tos;
	uint16_t		len;
	uint16_t		id;
	uint16_t		flag_frags;
	uint8_t			ttl;
	uint8_t			proto;
	uint16_t		cksum;
	uint8_t			source[4];
	uint8_t			dest[4];
} __attribute__((packed));

int ip_handle_packet (struct NETPACKET* np);
struct NETPACKET* ip_build_packet (uint8_t proto, uint32_t dest, uint16_t pktlen, uint8_t* data);
int ip_transmit_packet (uint8_t proto, uint32_t dest, uint16_t pktlen, uint8_t* data);
int ip_transmit (uint32_t dest, struct NETPACKET* pkt);

#endif /* __IP_H__ */
