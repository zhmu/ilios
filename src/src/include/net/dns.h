/*
 * dns.h - ILIOS DNS IPv4 stack
 * (c) 2003 Rink Springer, BSD licensed
 *
 */
#include <sys/types.h>

#ifndef __DNS_H__
#define __DNS_H__

/* DNS_PORT is the official DNS port */
#define DNS_PORT 53

/* DNS_FLAG_RD is the Recursion Desired flag */
#define DNS_FLAG_RD 0x01

/* DNS_FLAG_RESPONSE is the response flag */
#define DNS_FLAG_RESPONSE 0x80

/* DNS_CLASS_IN is the internet class */
#define DNS_CLASS_IN 1

/* DNS_QTYPE_HOST returns a host address */
#define DNS_QTYPE_HOST 1
#define DNS_QTYPE_CNAME 5
#define DNS_QTYPE_ANY 255

struct DNS_HEADER {
	uint16_t id;
	uint8_t  opcode_flags;
	uint8_t  ra_z_rcode;
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
} __attribute__((packed));

void dns_init();
uint32_t dns_query (char* query);

#endif /* __DNS_H__ */

/* vim:set ts=2 sw=2: */
