/*
 * cksum.h - ILIOS IPv4 checksumming
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 */
#include <sys/types.h>
#include <ipv4/ip.h>
#include <ipv4/icmp.h>

#ifndef __CKSUM_H__
#define __CKSUM_H__

uint16_t ipv4_cksum (char* data, int len);
int ipv4_check_ipcksum (IP_HEADER* hdr);
int ipv4_check_icmpcksum (ICMP_HEADER* hdr, int len);

unsigned short ip_fast_csum(unsigned char * iph, unsigned int ihl);

#endif /* __CKSUM_H__ */
