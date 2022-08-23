/*
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This is the file which handles IP.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <lib/lib.h>
#include <md/timer.h>
#include <netipv4/arp.h>
#include <netipv4/cksum.h>
#include <netipv4/ip.h>
#include <netipv4/icmp.h>
#include <netipv4/route.h>
#include <netipv4/tcp.h>
#include <netipv4/udp.h>

/*
 * This will route IP packet [np] as needed.
 */
int
ip_route (struct NETPACKET* np) {
	struct DEVICE* dev;
	struct IP_HEADER* iphdr = (struct IP_HEADER*)(np->data);
	struct ARP_RECORD* arp;
	uint32_t dest = ipv4_conv_addr (iphdr->dest);
	uint32_t chk;

	/* look up the destination */
	dev = route_find_device (dest);
	if (dev == NULL)
		/* this failed. (XXX: send ICMP message?) */
		return 0;

#if 0
	/* broadcast? */
	if (ipv4_is_broadcast (dest))
		/* yes. silently drop it */
		return 0;
#endif

	/* fetch the hardware address */
	arp = arp_fetch_address (dest);
	if (arp == NULL)
		/* this failed. drop the packet (XXX) */
		return 0;

	/* decrement the TTL */
	iphdr->ttl--;

	/* TTL of zero or one ? */
	if ((iphdr->ttl == 0) || (iphdr->ttl == 1))
		/* yes. drop the packet (XXX: send ICMP message) */
		return 0;

	/* update the checksum */
	iphdr->cksum = 0; chk = ipv4_cksum ((char*)iphdr, ((iphdr->version_ihl) &
0x0f) * 4, 0);
	iphdr->cksum = chk;

	/* got it! send it out */
	network_xmit_packet (dev, np, arp->hw_addr);

	/* don't drop the packet! */
	return 1;
}

/*
 * This will handle incoming IP packet [np]. It will return
 * zero on failure or non-zero on success.
 */
int
ip_handle_incoming (struct NETPACKET* np) {
	struct IP_HEADER* iphdr = (struct IP_HEADER*)(np->data);

	/* TCP packet? */
	if (iphdr->proto == IP_PROTO_TCP)
		/* yes. handle it */
		return tcp_handle_packet (np);

	/* UDP packet? */
	if (iphdr->proto == IP_PROTO_UDP)
		/* yes. handle it */
		return udp_handle_packet (np);

	/* we discard incoming packets */
	return 0;
}

/*
 * This will build an IP packet to IPv4 destination [dest]. The complete
 * packet size (*EXCLUDING* IP header) should be [len], and the data should be
 * in [data] It will return the network packet on success or NULL on failure.
 */
struct NETPACKET*
ip_build_packet (uint8_t proto, uint32_t dest, uint16_t pktlen, uint8_t* data) {
	struct NETPACKET* pkt;
	struct IP_HEADER* iphdr;
	uint16_t cksum;
	struct IPV4_ADDR* addr;
	struct ARP_RECORD* ar;
	struct DEVICE* dev = route_find_device (dest);

	/* do we have a device to route ? */
	if (dev == NULL)
		/* no. drop the packet */
		return 0;

	/* fetch the IP address from which we can reach this address */
	addr = route_find_ip (dev, dest);
	if (addr == NULL)
		/* this failed. drop the packet */
		return 0;

	/* fetch the address of the destination */
	ar = arp_fetch_address (dest);
	if (ar == NULL)
		/* it's not yet in the cache. bail out */
		return 0;

	/* allocate a network packet */
	pkt = network_alloc_packet (dev);
	if (pkt == NULL)
		/* out of network packets. drop the packet */
		return 0;

	/* append the data to send */
	if (data)
		kmemcpy ((uint8_t*)(pkt->data + sizeof (struct IP_HEADER)),
				data,
				pktlen);
	pktlen += sizeof (struct IP_HEADER);

	/* build the IP header */
	iphdr = (struct IP_HEADER*)pkt->data;
	iphdr->version_ihl = 0x40 | (sizeof (struct IP_HEADER) / 4);
	iphdr->tos = 0x10;
	iphdr->id = htons (arch_timer_get());
	iphdr->len = (pktlen >> 8) | ((pktlen & 0xff) << 8);
	iphdr->flag_frags = 0;
	iphdr->ttl = 64;
	iphdr->proto = proto;
	iphdr->source[0] = (addr->addr >> 24) & 0xff;
	iphdr->source[1] = (addr->addr >> 16) & 0xff;
	iphdr->source[2] = (addr->addr >>  8) & 0xff;
	iphdr->source[3] = (addr->addr      ) & 0xff;
	iphdr->dest[0]   = (dest   >> 24) & 0xff;
	iphdr->dest[1]   = (dest   >> 16) & 0xff;
	iphdr->dest[2]   = (dest   >>  8) & 0xff;
	iphdr->dest[3]   = (dest        ) & 0xff;
	iphdr->cksum = 0;	
	cksum = ipv4_cksum ((void*)iphdr, sizeof (struct IP_HEADER), 0);
	iphdr->cksum = cksum;

	/* this worked. return the packet */
	pkt->len = pktlen;
	return pkt;
}

/*
 * This will transmit a NETPACKET to IPv4 destination [dest]. It will return
 * the network packet on success or NULL on failure.
 */
int
ip_transmit (uint32_t dest, struct NETPACKET* pkt) {
	struct ARP_RECORD* ar;
	struct DEVICE* dev = route_find_device (dest);

	/* do we have a device to route ? */
	if (dev == NULL)
		/* no. drop the packet */
		return 0;

	/* fetch the address of the destination */
	ar = arp_fetch_address (dest);
	if (ar == NULL)
		/* it's not yet in the cache. bail out */
		return 0;

	/* send the packet */
	network_xmit_packet (dev, pkt, ar->hw_addr);

	/* victory */
	return 1;
}


/*
 * This will transmit an IP packet to IPv4 destination [dest]. The complete
 * packet size (*EXCLUDING* IP header) should be [len], and the data should be
 * in [data] It will return the network packet on success or NULL on failure.
 */
int
ip_transmit_packet (uint8_t proto, uint32_t dest, uint16_t pktlen, uint8_t* data) {
	struct NETPACKET* pkt;

	/* build the packet */
	pkt = ip_build_packet (proto, dest, pktlen, data);
	if (pkt == NULL)
		return 0;

	/* send it */
	return ip_transmit (dest, pkt);
}

/*
 * This will handle IP packet [np]. It will return
 * zero on failure or non-zero on success.
 *
 */
int
ip_handle_packet (struct NETPACKET* np) {
	struct IP_HEADER* iphdr = (struct IP_HEADER*)(np->data);

	/* IPv4 thing? */
	if ((iphdr->version_ihl >> 4) != 4)
		/* no. not our cup of tea */
		return 0;

	/* do we have a valid header? */
	if (ip_fast_csum (np->data, iphdr->version_ihl & 0x0f) != 0)
		/* no. silently drop it */
		return 0;

	/* ICMP thing? */
	if (iphdr->proto == IP_PROTO_ICMP)
		/* yes. have ICMP handle it */
		return icmp_handle_packet (np);

	/* are we bound to this address? */
	if (ipv4_is_bound (ipv4_conv_addr (iphdr->dest)) == np->device)
		/* yes. pass it through to the incoming handler*/
		return ip_handle_incoming (np);

	/* need to route the packet? */
	if (!ipv4_routing)
		/* no. drop it */
		return 0;

	/* route the packet */
	return ip_route (np);
}

/* vim:set ts=2 sw=2 tw=78: */
