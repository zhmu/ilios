/*
 *
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This is the checksumming include file.
 *
 */
#include <sys/types.h>
#include <sys/network.h>

#ifndef __CKSUM_H__
#define __CKSUM_H__

uint16_t ipv4_cksum (char* data, int len, int cksum);
unsigned short ip_fast_csum(unsigned char * iph, unsigned int ihl);

#endif /* __CKSUM_H__ */
