/*
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This is the file which handles ICMP.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <netipv4/arp.h>
#include <netipv4/cksum.h>
#include <netipv4/icmp.h>
#include <netipv4/ip.h>
#include <netipv4/route.h>
#include <lib/lib.h>

/*
 * This will handle ICMP Echo Request packet [np].
 */
int
icmp_handle_echorequest (struct NETPACKET* np) {
	struct ICMP_HEADER* ihdr = (struct ICMP_HEADER*)(np->data + sizeof (struct IP_HEADER));
	struct IP_HEADER* iphdr = (struct IP_HEADER*)(np->data);
	uint32_t src, dst;
	uint16_t c;

#if 0
	/* are we bound to this address? */
	if (!ipv4_is_ours (ipv4_conv_addr (iphdr->dest)))
		/* no. route the packet */
		return icmp_route (np);
#endif

	/* flip the source and dest IP addresses */
	src = *(uint32_t*)iphdr->source;
	dst = *(uint32_t*)iphdr->dest;
	*(uint32_t*)iphdr->source = dst;
	*(uint32_t*)iphdr->dest = src;

	/* update the ICMP header */
	ihdr->type = ICMP_TYPE_ECHORESPONSE;

	/*
	 * update the ICMP checksum
	 *
	 * XXX: since the only real difference is a change from 0 to 8 (ICMP type)
	 * this could be done much faster ...
	 */
	ihdr->cksum = 0;
	c = ipv4_cksum ((char*)ihdr, np->len - sizeof (struct ICMP_HEADER), 0);
	ihdr->cksum = c;

	/* go */
	network_xmit_packet (np->device, np, (char*)((ETHERNET_HEADER*)np->frame)->source);

	/* keep the packet (we're retransmitting it) */
	return 1;
}

/*
 * This will handle ICMP packet [np]. It will return
 * zero on failure or non-zero on success.
 *
 */
int
icmp_handle_packet (struct NETPACKET* np) {
	/*IP_HEADER* iphdr = (IP_HEADER*)(np->data);*/
	struct ICMP_HEADER* ihdr = (struct ICMP_HEADER*)(np->data + sizeof (struct IP_HEADER));

#if 0
	/* checksum OK? */
	if (!ipv4_check_icmpcksum (ihdr, iphdr->len - sizeof (IP_HEADER))) {
		kprintf ("icmp packet with bad checksum (%x)?\n", ihdr->cksum);
	}
#endif

	/* echo packet? */
 	if (ihdr->type == ICMP_TYPE_ECHOREQUEST)
		/* yes. handle it */
		return icmp_handle_echorequest (np);

#if 0
	/* echo reply packet? */
 	if (ihdr->type == ICMP_TYPE_ECHORESPONSE)
		/* yes. handle it */
		return icmp_route (np);
#endif

	/* unknown/unsupported type. discard it (XXX) */
	return 0;
}

/*
 * This will send an ICMP unreachable message to [dest], using code [code]
 * and [len] bytes of [data].
 */
int
icmp_send_unreachable (uint32_t dest, uint8_t code, uint8_t* header, uint8_t headerlen, uint8_t* data, uint8_t len) {
	struct ICMP_HEADER* icmphdr;
	char pkt[32 + sizeof (struct ICMP_HEADER)];
	uint16_t pktlen = sizeof (struct ICMP_HEADER) + headerlen + len;
	uint16_t cksum;

	/* build the ICMP message */
	icmphdr = (struct ICMP_HEADER*)pkt;
	icmphdr->type = ICMP_TYPE_UNREACHABLE;
	icmphdr->code = code;
	icmphdr->cksum = 0;
	icmphdr->ident = 0;
	icmphdr->seq = 0;

	/* copy the header and 8 bytes of data to the packet */
	kmemcpy (pkt + sizeof (struct ICMP_HEADER), header, headerlen);
	kmemcpy (pkt + sizeof (struct ICMP_HEADER) + headerlen, data, len);

	/* calculate the checksum */
	cksum = ipv4_cksum ((void*)icmphdr, sizeof (struct ICMP_HEADER) + headerlen + len, 0);
	icmphdr->cksum = cksum;

	/* build and transmit the packet */
	return ip_transmit_packet (IP_PROTO_ICMP, dest, pktlen, pkt);
}

/* vim:set ts=2 sw=2 tw=78: */
