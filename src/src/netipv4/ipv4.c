/*
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This is the main include file.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <net/socket.h>
#include <netipv4/arp.h>
#include <netipv4/ipv4.h>
#include <netipv4/ip.h>
#include <netipv4/icmp.h>
#include <netipv4/route.h>
#include <lib/lib.h>

int ipv4_routing = 0;

/*
 * This will handle IPv4 packet [np]. It will return
 * zero on failure or non-zero on success.
 *
 */
int
ipv4_handle_packet (struct NETPACKET* np) {
	ETHERNET_HEADER* eh = (ETHERNET_HEADER*)np->frame;
	uint16_t type = (eh->type[0] << 8) | eh->type[1];

	/* figure out the packet type */
	switch (type) {
		case ETHERTYPE_IP: /* IP packet */
		                   return ip_handle_packet (np);
	 case ETHERTYPE_ARP: /* ARP packet */
		                   return arp_handle_packet (np);
	}

	/* what's this? */
	return 0;
}

/*
 * This will convert a 4 byte IP address to an unsigned integer.
 */
uint32_t
ipv4_conv_addr (uint8_t* addr) {
	return (addr[0] << 24) | (addr[1] << 16) | (addr[2] <<  8) | (addr[3]);
}

/*
 * This checks whether [addr] is an valid address for [ap], and doesn't
 * conflict with known network or broadcast addresses. It will return zero
 * if [addr] is not valid, or non-zero if it is valid.
 */
int
ipv4_is_valid (uint32_t addr, struct IPV4_ADDR* ap) {
	/* network addresses are not allowed */
	if (addr == (ap->addr & ap->netmask)) return 0;

	/* broadcast addresses are not allowed either */
	if (addr == (addr | ~ap->netmask)) return 0;

	/* seems fine */
	return 1;
}

/*
 * This will return the device to which [addr] is bound. It will return zero on
 * failure or non-zero on success.
 */
struct DEVICE*
ipv4_is_bound (uint32_t addr) {
	struct DEVICE* dev = coredevice;

	/* scan all network cards */
	while (dev) {
		/* bound? */
		if (ipv4_is_device_bound (addr, dev) != NULL)
			/* yes. got it */
			return dev;

		/* next */
		dev = dev->next;
	}

	/* not bound, sorry */
	return NULL;
}

/*
 * This checks whether [addr] is bound to IPv4 interface [dev]. It will
 * return NULL on failure or 
 */
struct IPV4_ADDR*
ipv4_is_device_bound (uint32_t addr, struct DEVICE* dev) {
	int i;

	/* wade through all addresses */
	for (i = 0; i < IPV4_MAX_ADDR; i++)
		/* match? */
		if (dev->ipv4conf.address[i].addr == addr)
			/* yes. woohoo */
			return &dev->ipv4conf.address[i];

	/* too bad */
	return NULL;
}

/*
 * This will add IP address [addr] with netmask [mask] to device [dev]. THis
 * will return zero on failure or non-zero on success.
 */
int
ipv4_add_address (struct DEVICE* dev, uint32_t addr, uint32_t mask) {
	int i;

	/* wade through all addresses */
	for (i = 0; i < IPV4_MAX_ADDR; i++)
		/* used? */
		if (!dev->ipv4conf.address[i].addr) {
			/* no. now, it is! */
			dev->ipv4conf.address[i].addr = addr;
			dev->ipv4conf.address[i].netmask = mask;

			/* send out an ARP request, to ensure this address is available */
			arp_send_request (addr, dev);

			/* permanently add it to our own ARP cache */
			arp_add_record (addr, dev->ether.hw_addr, dev, ARP_FLAGS_PERMANENT);

			/* add a route */
			route_add (dev, addr, mask, 0, ROUTE_FLAG_PERM);
			return 1;
		}

	/* out of addresses. too bad */
	return 0;
}

/*
 * This will remove IP address [addr] from device [dev]. It will return zero on
 * failure or non-zero on success.
 */
int
ipv4_remove_address (struct DEVICE* dev, uint32_t addr) {
	int i;

	/* wade through all addresses */
	for (i = 0; i < IPV4_MAX_ADDR; i++)
		/* guilty one? */
		if (dev->ipv4conf.address[i].addr == addr) {
			/* yes.  kill the route first (we need the mask for that) */
			route_remove (addr, dev->ipv4conf.address[i].netmask);

			/* zap it */
			kmemset (&dev->ipv4conf.address[i], 0, sizeof (struct IPV4_ADDR));

			/* kill the permanent ARP record */
			arp_remove_record (addr, dev);
			return 1;
		}

	/* not found */
	return 0;
}

/*
 * ipv4_purge_device (struct DEVICE* dev)
 *
 * This will purge all addresses and ARP entries for device [dev].
 *
 */
void
ipv4_purge_device (struct DEVICE* dev) {
	/* bye to addresses */
	kmemset (&dev->ipv4conf.address, 0, IPV4_MAX_ADDR * sizeof (struct IPV4_ADDR));

	/* zap the ARP cache of this interface */
	arp_flush_device (dev);
}

/*
 * This will be called periodically to sync IPv4.
 */
void
ipv4_tick() {
}

/*
 * This will return the guessed netmask for [addr].
 */
uint32_t
ipv4_guess_netmask (uint32_t addr) {
	uint32_t addrtop = (addr >> 24);

	/* wade through all classes */
	if ((addrtop & 0x80) == 0)
		/* class A: 0.0.0.0 - 127.255.255.255 */
		return 0xFF000000;
	else if ((addrtop & 0x80) && ((addrtop & 0x40) == 0))
		/* class B: 128.0.0.0 - 191.25.255.255 */
		return 0xFFFF0000;
	else
		/* class C: 192.0.0.0 - 223.255.255.255 */
		/* class D: 224.0.0.0 - 239.255.255.255 */
		/* class E: 240.0.0.0 - 255.255.255.255 */
		return  0xFFFFFF00;
}

/*
 * This will enable or disable IPv4 routing.
 */
void
ipv4_set_routing (int val) {
	ipv4_routing = val;
}

/*
 * This will initialize IPv4.
 */
void
ipv4_init() {
	socket_init();
	arp_init();
	route_init();
}

/* vim:set ts=2 sw=2 tw=78: */
