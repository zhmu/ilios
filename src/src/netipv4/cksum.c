/*
 * cksum.c - XeOS IPv4 checksumming code
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 * This module handles IPv4 checksums.
 *
 */
#include <sys/types.h>
#include <netipv4/ipv4.h>

#if 0
/*
 * ipv4_cksum (char* pkt, int size)
 *
 * This will calculate the IP header checksum of [pkt]
 *
 * From http://www.netfor2.com/ipsum.htm
 *
 */
uint16_t
ipv4_cksum (char* data, int len) {
	uint8_t* b = (uint8_t*)data;
	uint32_t sum = 0;
	uint16_t i;
	uint16_t word16;
    
	/* make 16 bit words out of every two adjacent 8 bit words in the packet
	   and add them up */
	for (i=0;i<len;i=i+2){
		word16 =((b[i]<<8)&0xFF00)+(b[i+1]&0xFF);
		sum = sum + (uint32_t) word16;	
	}
	
	/* take only 16 bits out of the 32 bit sum and add up the carries */
	while (sum>>16)
	  sum = (sum & 0xFFFF)+(sum >> 16);

	/* one's complement the result*/
	sum = ~sum;

	/* swap em */
	sum = ((sum & 0xff00) >> 8) | ((sum & 0xff) << 8);
	return sum;
}
#else
/*
 * ipv4_cksum (char* pkt, int size)
 *
 * This will calculate the IP header checksum of [pkt]
 *
 * From tcpdump
 *
 */
uint16_t
ipv4_cksum (char* data, int len, int csum) {
  int nleft = len;
  const uint16_t *w = (uint16_t*)data;
  uint16_t answer;
	int sum = csum;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
 	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1)
		sum += ((
		/*	((((*(uint8_t*)w & 0xff) << 8) |*/
			(*(uint8_t*)w & 0xff)) << 8);

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 *
	 */
	sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
	sum += (sum >> 16);                     /* add carry */
	answer = ~sum;                          /* truncate to 16 bits */

	return answer;
}
#endif

/*
 *	This is a version of ip_compute_csum() optimized for IP headers,
 *	which always checksum on 4 octet boundaries.
 *
 *	By Jorge Cwik <jorge@laser.satlink.net>, adapted for linux by
 *	Arnt Gulbrandsen.
 */
unsigned short ip_fast_csum(unsigned char * iph,
					  unsigned int ihl)
{
	unsigned int sum;

	__asm__ __volatile__(
	    "movl (%1), %0	;\n"
	    "subl $4, %2	;\n"
	    "jbe 2f		;\n"
	    "addl 4(%1), %0	;\n"
	    "adcl 8(%1), %0	;\n"
	    "adcl 12(%1), %0	;\n"
"1:	    adcl 16(%1), %0	;\n"
	    "lea 4(%1), %1	;\n"
	    "decl %2		;\n"
	    "jne 1b		;\n"
	    "adcl $0, %0	;\n"
	    "movl %0, %2	;\n"
	    "shrl $16, %0	;\n"
	    "addw %w2, %w0	;\n"
	    "adcl $0, %0	;\n"
	    "notl %0		;\n"
"2:				;\n"
	/* Since the input registers which are loaded with iph and ipl
	   are modified, we must also specify them as outputs, or gcc
	   will assume they contain their original values. */
	: "=r" (sum), "=r" (iph), "=r" (ihl)
	: "1" (iph), "2" (ihl));
	return(sum);
}

/* vim:set ts=2 sw=2: */
