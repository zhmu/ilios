/*
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This is the file which handles UDP.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <sys/network.h>
#include <lib/lib.h>
#include <md/timer.h>
#include <netipv4/arp.h>
#include <netipv4/cksum.h>
#include <netipv4/icmp.h>
#include <netipv4/ip.h>
#include <netipv4/ipv4.h>
#include <netipv4/icmp.h>
#include <netipv4/route.h>
#include <netipv4/udp.h>

/*
 * This will handle incoming UDP packet [np]. It will return
 * zero on failure or non-zero on success.
 */
int
udp_handle_packet (struct NETPACKET* np) {
	struct IP_HEADER* iphdr = (struct IP_HEADER*)(np->data);
	struct UDP_HEADER* udphdr = (struct UDP_HEADER*)(np->data + sizeof (struct IP_HEADER));
	uint32_t addr = ipv4_conv_addr (iphdr->source);
	uint16_t dest_port = (udphdr->dest[0] << 8) | udphdr->dest[1];
	uint16_t len = (udphdr->length[0] << 8) | udphdr->length[1];
	struct SOCKET* s = socket_find (SOCKET_TYPE_UDP4, dest_port);

	/*
	kprintf ("UDP incoming from %I to %I: src port %u dest %u\n",
			ipv4_conv_addr (iphdr->source), ipv4_conv_addr (iphdr->dest),
			source, dest);
	*/

	/* do we have a socket bound to this? */
	if (s == NULL) {
		/* no. send ICMP unreachable port message */
		icmp_send_unreachable (addr, ICMP_CODE_PORTUNREACHABLE, (uint8_t*)iphdr, sizeof (struct IP_HEADER), (uint8_t*)udphdr, 8);
		return 0;
	}

	/* do we have a callback routine? */
	if (s->callback != NULL)
		/* yes. call it */
		s->callback (s, np->device, addr, np->data + sizeof (struct IP_HEADER) + sizeof (struct UDP_HEADER), len - sizeof (struct UDP_HEADER));

	/* all done. no futher need to keep the packet into memory */
	return 0;
}

/*
 * This will transmit [len] bytes of [data] to port [port] of IP address [dest].
 * It will use the port numbers in socket [s]. it will return 0 on failure or
 * 1 on success.
 */
int
udp_xmit_packet (struct SOCKET* s, uint32_t dest, uint16_t port, uint8_t* data, uint32_t len) {
	struct DEVICE* dev = route_find_device (dest);
	struct IPV4_ADDR* addr;
	struct ARP_RECORD* ar;

	/* do we have a device to route ? */
	if (dev == NULL)
		/* no. drop the packet */
		return 0;

	/* fetch the IP address to route */
	addr = route_find_ip (dev, dest);
	if (addr == NULL)
		/* this failed. drop the packet */
		return 0;

	/* fetch the address of the destination */
	ar = arp_fetch_address (dest);
	if (ar == NULL)
		/* it's not yet in the cache. bail out */
		return 0;

	/* fetch the IP address to route */
	addr = route_find_ip (dev, dest);
	if (addr == NULL)
		/* this failed. drop the packet */
		return 0;

	/* fetch the address of the destination */
	ar = arp_fetch_address (dest);
	if (ar == NULL)
		/* it's not yet in the cache. bail out */
		return 0;

	return udp_xmit_packet_ex (dev, ar->hw_addr, addr->addr, dest, s->port, port, data, len);
}

/*
 * This will transmit [len] bytes of [data] from port [srcport] to port [port]
 * of IP address [dest]. It will use the port numbers in socket [s]. it will
 * return 0 on failure or 1 on success.
 */
int
udp_xmit_packet_ex (struct DEVICE* dev, uint8_t* hwaddr, uint32_t source, uint32_t dest, uint16_t srcport, uint16_t port, uint8_t* data, uint32_t len) {
	struct NETPACKET* pkt;
	struct IP_HEADER* iphdr;
	struct UDP_HEADER* udphdr;
	uint16_t pktlen = (sizeof (struct IP_HEADER) + sizeof (struct UDP_HEADER) + len);
	uint16_t cksum;

	/* allocate a network packet */
	pkt = network_alloc_packet (dev);
	if (pkt == NULL)
		/* out of network packets. drop the packet */
		return 0;

	/* copy the data */
	kmemcpy ((uint8_t*)(pkt->data + sizeof (struct IP_HEADER) + sizeof (struct UDP_HEADER)), data, len);

	/* build the IP header */
	iphdr = (struct IP_HEADER*)pkt->data;
	iphdr->version_ihl = 0x40 | (sizeof (struct IP_HEADER) / 4);
	iphdr->tos = 0x10;
	iphdr->id = htons (arch_timer_get());
	iphdr->len = (pktlen >> 8) | ((pktlen & 0xff) << 8);
	iphdr->flag_frags = 0;
	iphdr->ttl = 64;
	iphdr->proto = IP_PROTO_UDP;
	iphdr->source[0] = (source >> 24) & 0xff;
	iphdr->source[1] = (source >> 16) & 0xff;
	iphdr->source[2] = (source >>  8) & 0xff;
	iphdr->source[3] = (source      ) & 0xff;
	iphdr->dest[0]   = (dest   >> 24) & 0xff;
	iphdr->dest[1]   = (dest   >> 16) & 0xff;
	iphdr->dest[2]   = (dest   >>  8) & 0xff;
	iphdr->dest[3]   = (dest        ) & 0xff;
	iphdr->cksum = 0;	
	cksum = ipv4_cksum ((void*)iphdr, sizeof (struct IP_HEADER), 0);
	iphdr->cksum = cksum;

	/* build the UDP header */
	udphdr = (struct UDP_HEADER*)(pkt->data + sizeof (struct IP_HEADER));
	udphdr->source[0] = (srcport >> 8) & 0xff; udphdr->source[1] = (srcport & 0xff);
	udphdr->dest[0] = (port >> 8) & 0xff; udphdr->dest[1] = (port & 0xff);
	udphdr->length[0] = ((len + sizeof (struct UDP_HEADER)) >> 8) & 0xff;
	udphdr->length[1] = ((len + sizeof (struct UDP_HEADER))       & 0xff);
	udphdr->cksum[0] = 0; udphdr->cksum[1] = 0;
#if 0
	cksum = ipv4_cksum ((void*)pkt->data, pktlen);
	iphdr->cksum = cksum;
#endif

	/* off it goes! */
	pkt->len = pktlen;
	network_xmit_packet (dev, pkt, hwaddr);
	return 1;
}

/* vim:set ts=2 sw=2 tw=78: */
