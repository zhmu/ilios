/*
 * ILIOS i386 status screen
 * (c) 2003 Rink Springer
 *
 * This is the file which displays the i386 status
 * screen.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <net/dns.h>
#include <lib/lib.h>
#include <net/socket.h>
#include <netipv4/arp.h>
#include <md/interrupts.h>
#include <md/console.h>
#include <sys/network.h>

extern addr_t tty_videobase;
extern uint32_t offs;

/*
 * i386_status()
 */
void
i386_status() {
	addr_t base = tty_videobase + 4000;
	int oldints = arch_interrupts (DISABLE);
	int i, oldoffs = offs;
	struct DEVICE* dev = coredevice;

	/* blank the screen */
	for (i = 0; i < 4000; i += 2)
		*(uint16_t*)(base + i) = (0x1e << 8) | 0x20;

	/* fix the offset */
	offs = 0; tty_videobase = base;
	kprintf ("ILIOS status\n\n");

	/* walk through all devices */
	while (dev) {
		/* display the name */
		kprintf ("%s: ", dev->name);
		if (dev->resources.port) kprintf ("port %x ", dev->resources.port);
		if (dev->resources.irq) kprintf ("irq %x ", dev->resources.irq);

		/* show the MAC address */
		if (!((dev->ether.hw_addr[0] == 0) && (dev->ether.hw_addr[1] == 0) &&
		    (dev->ether.hw_addr[2] == 0) && (dev->ether.hw_addr[3] == 0) &&
			  (dev->ether.hw_addr[4] == 0) && (dev->ether.hw_addr[5] == 0))) {
			kprintf (" mac address %x:%x:%x:%x:%x:%x ",
				dev->ether.hw_addr[0], dev->ether.hw_addr[1], dev->ether.hw_addr[2], 
				dev->ether.hw_addr[3], dev->ether.hw_addr[4], dev->ether.hw_addr[5]);
		}

		/* show rx and tx */
		kprintf ("\n");
		kprintf ("     received: %lu frames, %lu bytes\n", dev->rx_frames, dev->rx_bytes);
 		kprintf ("     transmitted: %lu frames, %lu bytes\n", dev->tx_frames, dev->tx_bytes);

		/* show the IP address */
		for (i = 0; i < IPV4_MAX_ADDR; i++)
			/* got a valid IP address here? */
			if ((dev->ipv4conf.address[i].addr)) 
				/* yes. display the address */
				kprintf ("    ip address %I netmask %I\n",
					dev->ipv4conf.address[i].addr,
					dev->ipv4conf.address[i].netmask);

		/* next */
		dev = dev->next;
	}

	/* ARP table */
	kprintf ("ARP table:\n");
		/* wade through the entire table */
		for (i = 0; i < ARP_CACHE_SIZE; i++)
			/* address here? */
			if (arp_cache[i].address) {
				/* yes. display it */
				kprintf ("%I     %x:%x:%x:%x:%x:%x   %s  %s\n",
						arp_cache[i].address,
						arp_cache[i].hw_addr[0],	
						arp_cache[i].hw_addr[1],	
						arp_cache[i].hw_addr[2],	
						arp_cache[i].hw_addr[3],	
						arp_cache[i].hw_addr[4],	
						arp_cache[i].hw_addr[5],
						arp_cache[i].device->name,
						/*(arp_cache[i].flags & ARP_FLAG_PERM) ? "[permanent]" : */"");
			}
		
	/* restore interrupts and offset */
	offs = oldoffs; tty_videobase = (base - 4000);
	arch_interrupts (oldints);
}

/* vim:set ts=2 sw=2: */
