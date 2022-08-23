/*
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This will deal with ARP packets, by RFC 826.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <sys/kmalloc.h>
#include <netipv4/arp.h>
#include <netipv4/ipv4.h>
#include <netipv4/route.h>
#include <lib/lib.h>

struct ARP_RECORD* arp_cache;

/*
 * This will send an ARP Request Packet for IP [addr]. If [fdev] is NULL, a
 * suitable device will be found to search for the address, otherwise [fdev]
 * will be used. It will return zero on failure or non-zero on success.
 */
int
arp_send_request (uint32_t addr, struct DEVICE* fdev) {
	struct NETPACKET* pkt;
	struct ARP_PACKET* arp;
	ETHERNET_HEADER* eh;
	struct IPV4_ADDR* ia;
	uint32_t qaddr;
	struct DEVICE* dev;

	/* device given? */
	if (fdev == NULL) {
		/* no. find the device required to query this address */
		dev = route_find_device (addr);
		if (dev == NULL)
			/* this failed. bail out */
			return 0;

		/* look up the most appropriate address */
		ia = route_find_ip (dev, addr);
		if (ia == NULL)
			/* we don't know this address. bail out */
			return 0;

		/* fill out the query address */
		qaddr = ia->addr;
	} else {
		/* yes. use this device */
		dev = fdev;

		/* no query address */
		qaddr = 0;
	}

	/* got a network packet? */
 	pkt = network_alloc_packet (dev);
	if (pkt == NULL)
		/* no. bail out */
		return 0;

	/* set the packet up */
	pkt->len = sizeof (struct ARP_PACKET);
	arp = (struct ARP_PACKET*)pkt->data;
	eh = (ETHERNET_HEADER*)pkt->frame;

	/* fill out the frame information */
	kmemset (eh->dest,   0xFF, ETHER_ADDR_LEN);
	kmemcpy (eh->source, dev->ether.hw_addr, ETHER_ADDR_LEN);
	eh->type[0] = 0x08; eh->type[1] = 0x06;

	/* fill out the ARP packet */
	kmemset (arp, 0, sizeof (struct ARP_PACKET));
	arp->hw_type[0] = (ARP_HWTYPE_ETH >> 8) & 0xff;
	arp->hw_type[1] = (ARP_HWTYPE_ETH & 0xff);
	arp->protocol[0] = (ETHERTYPE_IP >> 8) & 0xff;
	arp->protocol[1] = (ETHERTYPE_IP & 0xff);
	arp->hw_len = ETHER_ADDR_LEN;
	arp->proto_len = 4;
	arp->opcode[0] = (ARP_REQUEST >> 8) & 0xff;
	arp->opcode[1] = (ARP_REQUEST & 0xff);
	kmemcpy (arp->hw_source, dev->ether.hw_addr, ETHER_ADDR_LEN);
	kmemset (arp->hw_dest, 0xFF, ETHER_ADDR_LEN);

	/* fill out the IPv4 addresses */
	arp->source_addr[0] = (qaddr >> 24) & 0xff;
	arp->source_addr[1] = (qaddr >> 16) & 0xff;
	arp->source_addr[2] = (qaddr >>  8) & 0xff;
	arp->source_addr[3] = (qaddr      ) & 0xff;
	arp->dest_addr[0] = (addr >> 24) & 0xff;
	arp->dest_addr[1] = (addr >> 16) & 0xff;
	arp->dest_addr[2] = (addr >>  8) & 0xff;
	arp->dest_addr[3] = (addr      ) & 0xff;

	/* transmit the packet */
	network_xmit_frame (dev, pkt);
	return 1;
}

/*
 * This will send an ARP Reply packet for packet [arp] on device [dev]. It will
 * return zero on failure or non-zero on success.
 */
int
arp_send_reply (struct NETPACKET* np) {
	struct NETPACKET* pkt = network_alloc_packet (np->device);
	struct ARP_PACKET* rarp;
	struct ARP_PACKET* arp = (struct ARP_PACKET*)np->data;
	ETHERNET_HEADER* eh = (ETHERNET_HEADER*)pkt->frame;
	int i;

	/* got a network packet? */
	if (pkt == NULL)
		/* no. bail out */
		return 0;

	/* set the packet up */
	kmemcpy (pkt->frame, np->frame, np->len + np->header_len);
	rarp = (struct ARP_PACKET*)pkt->data;
	pkt->len = sizeof (struct ARP_PACKET); pkt->header_len = sizeof (ETHERNET_HEADER);

	/* MAC address swap (bother frame header as ARP packet) */
	for (i = 0; i < 6; i++) {
		pkt->frame[i]      = np->frame[i + 6];
		pkt->frame[i + 6]  = np->frame[i];
		rarp->hw_source[i] = np->device->ether.hw_addr[i];
		rarp->hw_dest[i]   = eh->source[i];
	}

	/* this is a reply */
	rarp->opcode[0] = (ARP_REPLY >> 8) & 0xff;
	rarp->opcode[1] = (ARP_REPLY & 0xff);

	/* flip the source and dest IP addresses */
	for (i = 0; i < 4; i++) {
		rarp->source_addr[i] = arp->dest_addr[i];
		rarp->dest_addr[i]   = arp->source_addr[i];
	}
	/* send the frame */
	network_xmit_frame (np->device, pkt);

	/* we return zero because we allocate a new packet. therefore, the old one
	 * can be freed happily */
	return 0;
}

/*
 * This will handle ARP Request packet [np].
 */
int
arp_handle_request (struct NETPACKET* np) {
	struct ARP_PACKET* arp = (struct ARP_PACKET*)np->data;
	uint32_t addr = ipv4_conv_addr (arp->dest_addr);
	struct IPV4_ADDR* ia;

	/* are we bound to the destination address? */
	if (ipv4_is_bound (addr) == NULL)
		/* no. don't answer the query */
		return 0;

	/* look up the most appropriate address */
	ia = route_find_ip (np->device, addr);
	if (ia == NULL)
		/* we don't know this address. bail out */
		return 0;

	/* is the address reserved? */
	if (!ipv4_is_valid (addr, ia))
		/* yes. refuse to answer */
		return 0;

	/* we can answer this query. do it */
	return arp_send_reply (np);
}

/*
 * This will handle ARP Reply packet [np]. It will return zero on failure or
 * non-zero on success.
 */
int
arp_handle_reply (struct NETPACKET* np) {
	struct ARP_PACKET* arp = (struct ARP_PACKET*)np->data;
	uint32_t addr = ipv4_conv_addr (arp->source_addr);
	struct IPV4_ADDR* ia;

	/* look up the most appropriate address */
	ia = route_find_ip (np->device, ipv4_conv_addr (arp->dest_addr));
	if (ia == NULL)
		/* we don't know this address. bail out */
		return 0;

	/* is the address reserved? */
	if (!ipv4_is_valid (addr, ia))
		/* yes. refuse to answer */
		return 0;

	/* add or update the record */
	arp_update_record (addr, arp->hw_source, np->device);

	/* we return zero because the packet can be discarded */
	return 0;
}

/*
 * This will handle ARP packet [np].
 */
int
arp_handle_packet (struct NETPACKET* np) {
	struct ARP_PACKET* arp = (struct ARP_PACKET*)np->data;
	uint16_t opcode = (arp->opcode[0] << 8) | arp->opcode[1];

	/* sanity checks */
	if (arp->hw_len != ETHER_ADDR_LEN) return 0;
	if (arp->proto_len != 4) return 0;

	/* request? */
	switch (opcode) {
		case ARP_REQUEST: /* ARP request */
		                  return arp_handle_request (np);
		  case ARP_REPLY: /* ARP reply */
		                  return arp_handle_reply (np);
	}

	/* ??? */
	return 0;
}

/*
 * This will initialize the ARP cache.
 *
 */
void
arp_init() {
	/* allocate memory for the ARP cache */
	arp_cache = (struct ARP_RECORD*)kmalloc (NULL, sizeof (struct ARP_RECORD) * ARP_CACHE_SIZE, 0);

	/* clear the cache */
	kmemset (arp_cache, 0, sizeof (struct ARP_RECORD) * ARP_CACHE_SIZE);
}

/*
 * This will add an ARP cache record for host [h] on device [dev] with
 * hardware address [hw] and flags [fl]. It will return zero on failure or
 * non-zero on success.
 *
 */
int
arp_add_record (uint32_t h, char* hw, struct DEVICE* dev, uint8_t fl) {
	int i;

	/* wade through the entire ARP table */
	for (i = 0; i < ARP_CACHE_SIZE; i++)
		/* available? */
		if (arp_cache[i].device == NULL) {
			/* yes. modify this record */
			arp_cache[i].address = h;
			arp_cache[i].flags = fl;
			arp_cache[i].device = dev;
			arp_cache[i].timestamp = 0; /* TODO */
			kmemcpy (&arp_cache[i].hw_addr, hw, ETHER_ADDR_LEN);

			/* all done */
			return 1;
		}

	/* out of entries! */
	return 0;
}

/*
 * This will remove ARP cache entry [h] on host [dev]. It will return zero on
 * failure or non-zero on success.
 *
 */
int
arp_remove_record (uint32_t h, struct DEVICE* dev) {
	int i;

	/* wade through the entire ARP table */
	for (i = 0; i < ARP_CACHE_SIZE; i++)
		/* match? */
		if ((arp_cache[i].address == h) && (arp_cache[i].device == dev)) {
			/* yes. zap it */
			kmemset (&arp_cache[i], 0, sizeof (struct ARP_RECORD));
			return 1;
		}

	/* no such entry */
	return 0;
}

/*
 * This will return the host entry for [h], or NULL if is doesn't exist.
 */
struct ARP_RECORD*
arp_find_record (uint32_t h) {
	int i;

	/* wade through the entire ARP table */
	for (i = 0; i < ARP_CACHE_SIZE; i++)
		/* match? */
		if (arp_cache[i].address == h)
			/* yes. got it */
			return &arp_cache[i];

	/* no such entry */
	return NULL;
}

/*
 * This will update record [h] with hardware address [hw], or create it if
 * it doesn't exist. It will return zero on failure or non-zero on success.
 */
int
arp_update_record (uint32_t h, char* hw, struct DEVICE* dev) {
	struct ARP_RECORD* arp = arp_find_record (h);

	/* does the record exist? */
	if (arp != NULL) {
		/* yes. update it */
		kmemcpy (arp->hw_addr, hw, ETHER_ADDR_LEN);
		arp->device = dev;
	
		/* all done */
		return 1;
	} 

	/* add the new record */
	return arp_add_record (h, hw, dev, 0);
}

/*
 * This will flush the ARP table, removing all non-permanent entries.
 */
void
arp_flush() {
	int i;

	/* wade through the entire ARP table */
	for (i = 0; i < ARP_CACHE_SIZE; i++)
		/* not permanent? */
		if (!(arp_cache[i].flags & ARP_FLAGS_PERMANENT))
			/* yes. zap it */
			kmemset (&arp_cache[i], 0, sizeof (struct ARP_RECORD));
}

/*
 * This will return the hardware address for [addr]. If it does not
 * exist, an ARP query will be emitted and NULL will be returned. On
 * success, the address is returned.
 */
struct ARP_RECORD*
arp_fetch_address (uint32_t addr) {
	struct ARP_RECORD* rec = arp_find_record (addr);

	/* got a match? */
	if (rec != NULL)
		/* yes. return that */
		return rec;

	/* do an ARP query and fail */
	arp_send_request (addr, NULL);
	return NULL;
}

/*
 * This will flush the entire ARP table for a specific interface.
 */
void
arp_flush_device(struct DEVICE* dev) {
	int i;

	/* wade through the entire ARP table */
	for (i = 0; i < ARP_CACHE_SIZE; i++)
		/* is it for this device? */
		if (arp_cache[i].device == dev)
			/* yes. zap it */
			kmemset (&arp_cache[i], 0, sizeof (struct ARP_RECORD));
}

/* vim:set ts=2 sw=2 tw=78: */
