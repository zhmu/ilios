/*
 * network.c - XeOS Networking Subsystem
 * (c) 2003 Rink Springer, BSD licensed
 *
 * This code will handle networking packet transfers.
 *
 */
#include <sys/network.h>
#include <sys/tty.h>
#include <sys/kmalloc.h>
#include <lib/lib.h>
#include <md/interrupts.h>
#include <assert.h>
#include <config.h>

struct NETPACKET* network_netpacket;
struct NETPACKET* netpacket_first_avail;
struct NETPACKET* netpacket_last_avail;
struct NETPACKET* netpacket_todo_first;
struct NETPACKET* netpacket_todo_last;

int ipv4_handle_packet (struct NETPACKET* np);

int network_numbuffers = 0;

/*
 * This will initialize the networking system.
 */
void
network_init() {
	uint32_t i;
	size_t total, avail;
	struct NETPACKET* pkt;
	struct NETPACKET* pkt_next;

	/* figure out how much memory we have left */
	kmemstats (&total, &avail);

	/* do we have loads of free memory (> 2MB)? */
	if (avail > (2048 * 1024))
		/* yes. use most of the memory for packet buffers */
		network_numbuffers = (avail - (2048 * 1024)) / sizeof (struct NETPACKET);
	else
		/*  no. economy mode: use only 64 buffers */
		network_numbuffers = 64;

	/* allocate memory */
	network_netpacket = (struct NETPACKET*)kmalloc (NULL, (sizeof (struct NETPACKET) * network_numbuffers), 0);

	/* tone network buffers down if needed */
	while (network_netpacket == NULL) {
		/* got buffers? */
		if (network_numbuffers < 16)
			/* barely. complain */
			panic ("Unable to allocate reasonable amount of packet buffers");
	
		/* halve them */	
		network_numbuffers /= 2;

		/* give this a try */
		network_netpacket = (struct NETPACKET*)kmalloc (NULL, (sizeof (struct NETPACKET) * network_numbuffers), 0);
	}

	/* zero them out */
	kmemset (network_netpacket, 0, (sizeof (struct NETPACKET) * network_numbuffers));

	/* set the pointer to the first available netpacket */
	netpacket_first_avail = network_netpacket;

	/* build the chain of network packets */
	pkt = network_netpacket; pkt_next = pkt; pkt_next++;
	for (i = 0; i < network_numbuffers - 1; i++, pkt++, pkt_next++)
		pkt->next = pkt_next;
	netpacket_last_avail = pkt;
	pkt->next = NULL;

	/* nothing to do, yet */
	netpacket_todo_first = NULL;
	netpacket_todo_last = NULL;
}

/*
 * This will return a pointer to a new NETPACKET for device [dev]. It will
 * return a pointer to it on success or NULL on failure.
 */
struct NETPACKET*
network_alloc_packet (struct DEVICE* dev) {
	struct NETPACKET* pkt;
	int old_ints = arch_interrupts (DISABLE);
	
	/* got an available network packet? */
	if (netpacket_first_avail == NULL) {
		/* no. restore interrupts and bail out */
		arch_interrupts (old_ints);
		return NULL;
	}

	/* use this network packet */
	pkt = netpacket_first_avail;
	netpacket_first_avail = pkt->next;
	pkt->next = NULL;
	pkt->device = dev;

	/* set up internal pointers in this packet */
	pkt->data = pkt->frame + sizeof (ETHERNET_HEADER);
	pkt->header_len = sizeof (ETHERNET_HEADER);
	pkt->len = 0;

	/* restore the interrupts */
	arch_interrupts (old_ints);

	/* all done */
	return pkt;
}

/*
 * This will free packet [pkt].
 */
void
network_free_packet (struct NETPACKET* pkt) {
	int old_ints = arch_interrupts (DISABLE);

	/* put this packet as first in the chain of free netpackets */
	netpacket_last_avail->next = pkt;
	pkt->device = NULL;
	pkt->next = NULL;

	/* we got a new last packet */
	netpacket_last_avail = pkt;

	/* restore interrupts */
	arch_interrupts (old_ints);
}

/*
 * This will enqueue packet [pkt] for handling.
 */
void
network_queue_packet (struct NETPACKET* pkt) {
	int old_ints = arch_interrupts (DISABLE);

	/* update the header and data pointers */
	pkt->header_len = sizeof (ETHERNET_HEADER);
	pkt->data = (pkt->frame + sizeof (ETHERNET_HEADER));

	/* got anything in the todo list ? */
	if (netpacket_todo_first == NULL)
		/* no. now, we do */
		netpacket_todo_first = pkt;
	else
		/* yes. append it to the chain */
		netpacket_todo_last->next = pkt;

	/* build the chain ending */
	netpacket_todo_last = pkt;
	pkt->next = NULL;

	/* restore interrupts */
	arch_interrupts (old_ints);
}

/*
 * This will handle networking packet [pkt].
 */
void
network_handle_packet (struct NETPACKET* pkt) {
	/*
	 * Now, try all handlers. Therefore, add all handlers for packet types
	 * here.
	 *
	 * WARNING: Whenever a handler returns ZERO, it is assumed it cannot handle
	 * this packet and thus another handler should try.
	 *
	 * IF a handler returns NON-ZERO, it has handeled the packet. The packet
	 * MUST be freed by the handler in such a case!
	 */

	/* ipv4 it */
	if (ipv4_handle_packet (pkt))
		return;
	
	/* not handled. discard the packet */
	network_free_packet (pkt);
}

/*
 * This will handle all networking packets queue.
 */
void
network_handle_queue() {
	struct NETPACKET* pkt;
	int oldints = arch_interrupts (DISABLE);

	/* got packets to do? */
	if (netpacket_todo_first == NULL) {
		/* no. bail out */
		arch_interrupts (oldints);
		return;
	}

	/* fetch the next packet */
	pkt = netpacket_todo_first;
	netpacket_todo_first = pkt->next;
	arch_interrupts (oldints);

	/* handle this packet */
	network_handle_packet (pkt);
}

/*
 * This will send a frame cross the wire [len] bytes of packet [pkt]
 * to device [dev].
 */
void
network_xmit_frame (struct DEVICE* dev, struct NETPACKET* pkt) {
	int old_ints = arch_interrupts (DISABLE);

	/* got anything in the todo list ? */
	if (dev->xmit_packet_first == NULL)
		/* no. now, we do */
		dev->xmit_packet_first = pkt;
	else
		/* yes. append it to the chain */
		dev->xmit_packet_last->next = pkt;

	/* build the chain ending */
	dev->xmit_packet_last = pkt;
	pkt->next = NULL;

	/* restore interrupts */
	arch_interrupts (old_ints);

	/* send */
	dev->xmit (dev);
}

/*
 * network_xmit_frame (struct DEVICE* dev, void* pkt, size_t len)
 *
 * This will send [len] bytes of packet [pkt] to device [dev].
 *
 */
void
network_xmit_packet (struct DEVICE* dev, struct NETPACKET* pkt, void* addr) {
	ETHERNET_HEADER* eh = (ETHERNET_HEADER*)(pkt->frame);

	/* build the ethernet header */
	kmemcpy (eh->dest, addr, dev->addr_len);
	kmemcpy (eh->source, dev->ether.hw_addr, dev->addr_len);
	eh->type[0] = 0x08; eh->type[1] = 0x00;

	/* fill the header length out */
	pkt->header_len = sizeof (ETHERNET_HEADER);

	/* send */
	network_xmit_frame (dev, pkt);
}

/*
 * This will fetch the next transmit buffer packet of device [dev], or NULL if
 * there is no such one. The packet is removed from the transmit buffer.
 */
struct NETPACKET*
network_get_next_txbuf (struct DEVICE* dev) {
	int oldints = arch_interrupts (DISABLE);
	struct NETPACKET* pkt = dev->xmit_packet_first;
	
	/* got a packet to transmit? */
	if (pkt == NULL) {
		/* no. too bad */
		arch_interrupts (oldints);
		return NULL;
	}

	/* change the pointer */
	dev->xmit_packet_first = pkt->next;

	/* return the packet */
	arch_interrupts (oldints);
	return pkt;
}

/* vim:set ts=2 sw=2: */
