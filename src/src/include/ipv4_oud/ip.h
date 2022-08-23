/*
 * ipv4.h - ILIOS IPv4 IP
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 */
#include <sys/types.h>

#ifndef __IP_H__
#define __IP_H__

/* based on RFC 791 */
typedef struct {
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
} IP_HEADER __attribute__((packed));

#endif /* __IP_H__ */
