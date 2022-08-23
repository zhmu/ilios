/*
 * icmp.h - ILIOS IPv4 ICMP
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 */
#include <sys/types.h>
#include <sys/network.h>

#ifndef __ICMP_H__
#define __ICMP_H__

/* based on RFC 792 */
typedef struct {
	uint8_t			type;
	uint8_t			code;
	uint16_t		cksum;
	uint16_t		ident;
	uint16_t		seq;
} ICMP_HEADER __attribute__((packed));

#define ICMP_TYPE_ECHOREQUEST	8
#define ICMP_TYPE_ECHORESPONSE	0

int icmp_handle_packet (struct NETPACKET* np);

#endif /* __ICMP_H__ */
