/*
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This is the main include file.
 *
 */
#include <sys/types.h>
#include <sys/network.h>

#ifndef __ICMP_H__
#define __ICMP_H__

/* based on RFC 792 */
struct ICMP_HEADER {
	uint8_t			type;
	uint8_t			code;
	uint16_t		cksum;
	uint16_t		ident;
	uint16_t		seq;
} __attribute__((packed));

#define ICMP_TYPE_ECHORESPONSE	0
#define ICMP_TYPE_UNREACHABLE	3
#define ICMP_TYPE_ECHOREQUEST	8

#define ICMP_CODE_NETUNREACHABLE	0
#define ICMP_CODE_HOSTUNREACHABLE	1
#define ICMP_CODE_PROTOUNREACHABLE	2
#define ICMP_CODE_PORTUNREACHABLE	3

int icmp_handle_packet (struct NETPACKET* np);
int icmp_send_unreachable (uint32_t dest, uint8_t code, uint8_t* header, uint8_t headerlen, uint8_t* data, uint8_t len);

#endif /* __ICMP_H__ */
