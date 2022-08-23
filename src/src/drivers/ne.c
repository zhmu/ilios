/*
 * ne.c - XeOS NE2000 Card Driver
 * (c) 2002 Rink Springer, BSD licensed
 *
 * This code is based on OpenBSD's sys/dev/isa/if_ed.c file, (c) 1994, 1995
 * Charles M. Hannum, (c) 1993 David Greenman.
 *
 * This module handles all NE2000 device stuff.
 *
 */
#include <sys/device.h>
#include <sys/network.h>
#include <sys/kmalloc.h>
#include <sys/irq.h>
#include <sys/types.h>
#include <lib/lib.h>
#include <md/pio.h>
#include <md/interrupts.h>
#include "ne.h"
#include "ne_reg.h"

/* delay() is a dirty hack, FIXME! */
void delay (unsigned int i) { while (i--); }

/*
 * This will copy memory from the NIC to the buffer
 */
void
ne_pio_readmem (struct DEVICE* dev, uint16_t src, uint8_t* dst, uint16_t size) {
	int i;
	/*uint16_t c;*/
	NE_CONFIG* nc = (NE_CONFIG*)dev->data;

	/* select page 0 registers */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STA);

	/* round up to a word */
	if (size & 1)
		++size;

	/* set up dma byte count */
	outb (dev->resources.port + NE_P0_RBCR0, size);
	outb (dev->resources.port + NE_P0_RBCR1, size >> 8);

	/* set up source address in the nic memory */
	outb (dev->resources.port + NE_P0_RSAR0, src);
	outb (dev->resources.port + NE_P0_RSAR1, src >> 8);
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD0 | NE_CR_PAGE_0 | NE_CR_STA);

	/* off we go */
	if (nc->isa16bit) {
		/* 16 bit */
#if 0
		for (i = 0; i < (size >> 1); i++) {
			c = inw (dev->resources.port + 0x10);
			*(uint16_t*)dst = c;
			dst += 2;
		}
#else
		insw (dev->resources.port + 0x10, dst, (size >> 1));
#endif
	} else {
		/* 8 bit */
		for (i = 0; i < size; i++) {
			*dst = inb (dev->resources.port + 0x10);
			dst++;
		}
	}
}

/*
 * This will copy memory from the buffer to the NIC. [size] must be even!
 */
void
ne_pio_writemem (struct DEVICE* dev, uint8_t* src, uint16_t dst, uint16_t size) {
	NE_CONFIG* nc = (NE_CONFIG*)dev->data;
	/*uint16_t c;*/
	int maxwait = 100;
	int i;

	/* select page 0 registers */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STA);

	/* reset remote dma complete flag */
	outb (dev->resources.port + NE_P0_ISR, NE_ISR_RDC);

	/* set up dma byte count */
	outb (dev->resources.port + NE_P0_RBCR0, size);
	outb (dev->resources.port + NE_P0_RBCR1, size >> 8);

	/* set up destination address in nic memory */
	outb (dev->resources.port + NE_P0_RSAR0, dst);
	outb (dev->resources.port + NE_P0_RSAR1, dst >> 8);

	/* set remote DMA write */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD1 | NE_CR_PAGE_0 | NE_CR_STA);

	/* 16 bit? */
	if (nc->isa16bit) {
#if 0
		/* yes. do it */
		for (i = 0; i < (size >> 1); i++) {
			c = (*src) | (*(src + 1) << 8);
			outw (dev->resources.port + 0x10, c);
			src += 2;
		}
#else
		outsw (dev->resources.port + 0x10, src, (size >> 1));
#endif
	} else {
		/* no. 8 bit it is then */
		for (i = 0; i < size; i++) {
			outb (dev->resources.port + 0x10, *src);
			src++;
		}
	}

	/* wait for remote DMA completion */
	while (((inb (dev->resources.port + NE_P0_ISR) & NE_ISR_RDC) != NE_ISR_RDC) && (--maxwait));
}

/*
 * This will probe for a NE2000 card at offset [cf]. It will return 0 on
 * success or -1 on failure.
 */
int
modne_probe (struct DEVICE* dev) {
	NE_CONFIG* nc = (NE_CONFIG*)dev->data;
	uint8_t i;
	static uint8_t test_pattern[32] = "THIS is A memory TEST pattern";
	uint8_t test_buffer[32];
	uint8_t	romdata[16];
	int n;

	/* reset the card */
	i = inb (dev->resources.port + NE_PORT_RESET);
	outb (dev->resources.port + NE_PORT_RESET, i);
	delay (5000);

	/* ensure the nic is correctly reset */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STP);
	delay (5000);

	/* try to probe this card as an generic 8390 */
	if ((inb (dev->resources.port + NE_P0_CR) & (NE_CR_RD2 | NE_CR_TXP | NE_CR_STA | NE_CR_STP)) != (NE_CR_RD2 | NE_CR_STP))
		return -1;
	if ((inb (dev->resources.port + NE_P0_ISR) & NE_ISR_RST) != NE_ISR_RST)
		return -1;

	/* Now, we must see if this is a NE1000 or NE2000. This can be done by
		 reading/writing the memory */

	/* turn packet storage in the NIC's memory off */
	outb (dev->resources.port + NE_P0_RCR, NE_RCR_MON);

	/* initialize the DCR for byte operations */
	outb (dev->resources.port + NE_P0_DCR, NE_DCR_FT1 | NE_DCR_LS);
	outb (dev->resources.port + NE_P0_PSTART, 8192 >> NE_PAGE_SHIFT);
	outb (dev->resources.port + NE_P0_PSTOP, 16384 >> NE_PAGE_SHIFT);

	/* 8 bit mode */
	nc->isa16bit = 0;

	/* write a test pattern in byte mode. if this fails, then there is probably
		 no memory at 8k - which makes it likely this board is a NE2000 */
	ne_pio_writemem (dev, test_pattern, 8192, sizeof (test_pattern));
	ne_pio_readmem  (dev, 8192, test_buffer,  sizeof (test_buffer));

	/* match? */
	if (!kmemcmp (test_pattern, test_buffer, sizeof (test_pattern))) {
		/* yes. too bad, NE1000 is too ancient for us [XXX] */
		kprintf ("%s: sorry, but NE1000 is unsupported for now\n", dev->name);
		return -1;
	}

	/* not a ne1000. try ne2000 */
	outb (dev->resources.port + NE_P0_DCR, NE_DCR_WTS | NE_DCR_FT1 | NE_DCR_LS);
	outb (dev->resources.port + NE_P0_PSTART, 16384 >> NE_PAGE_SHIFT);
	outb (dev->resources.port + NE_P0_PSTOP, 32768 >> NE_PAGE_SHIFT);

	/* 16 bit mode */
	nc->isa16bit = 1;

	/* try to write the test pattern in word mode. if this fails, there is not
		 a ne2000 */
	ne_pio_writemem (dev, test_pattern, 16384, sizeof (test_pattern));
	ne_pio_readmem  (dev, 16384, test_buffer,  sizeof (test_buffer));

	/* match? */
	if (kmemcmp (test_pattern, test_buffer, sizeof (test_pattern))) {
		/* no. what's this? */
		kprintf ("%s: not NE1000, not NE2000 ... what's this ??\n", dev->name);
		return -1;
	}

	/* total of 16kb memory */
	nc->mem_size = 16384;

	/* calculate the offsets */
	nc->mem_start = 16384;
	nc->tx_page_start = nc->mem_size >> NE_PAGE_SHIFT;
	nc->mem_end = nc->mem_start + nc->mem_size;
	nc->txb_cnt = 2;

	nc->rec_page_start = nc->tx_page_start + nc->txb_cnt * NE_TXBUF_SIZE;
	nc->rec_page_stop = nc->tx_page_start + (nc->mem_size >> NE_PAGE_SHIFT);

	nc->mem_ring = nc->mem_start + ((nc->txb_cnt * NE_TXBUF_SIZE) << NE_PAGE_SHIFT);

	ne_pio_readmem (dev, 0, romdata, 16);
	for (n = 0; n < 6; n++)
		nc->addr[n] = romdata[n * 2];

	/* remove any pending irq's */
	outb (dev->resources.port + NE_P0_ISR, 0xff);

	/* victory */
	return 0;
}

/*
 * This will initialize initialize the card.
 */
void
ne_init_nic (struct DEVICE* dev) {
	NE_CONFIG* nc = (NE_CONFIG*)dev->data;
	int i;

	/* reset flags */
	nc->txb_inuse = 0;
	nc->txb_new = 0;
	nc->txb_next_tx = 0;

	/* set interface for page 0, remote dma complete, stopped */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STP);

	if (nc->isa16bit) {
		/* set fifo treshold to 8. no auto-init remote da, byte order x86, word
			 wide dma xfers */
		outb (dev->resources.port + NE_P0_DCR, NE_DCR_FT1 | NE_DCR_WTS | NE_DCR_LS);
	} else {
		/* same as above, but byte-wide DMA xfers */
		outb (dev->resources.port + NE_P0_DCR, NE_DCR_FT1 | NE_DCR_LS);
	}

	/* clear remote byte count registers */
	outb (dev->resources.port + NE_P0_RBCR0, 0);
	outb (dev->resources.port + NE_P0_RBCR1, 0);

	/* tell RCR to do nothing for now */
	outb (dev->resources.port + NE_P0_RCR, NE_RCR_MON);

	/* place nic in internal loopback mode */
	outb (dev->resources.port + NE_P0_TCR, NE_TCR_LB0);

	/* initialize receive buffer ring */
	outb (dev->resources.port + NE_P0_BNRY, nc->rec_page_start);
	outb (dev->resources.port + NE_P0_PSTART, nc->rec_page_start);
	outb (dev->resources.port + NE_P0_PSTOP, nc->rec_page_stop);

	/* clear all interrupts */
	outb (dev->resources.port + NE_P0_ISR, 0xff);

	/* enable the following interrupts: recv/xmit complete, recx/xmit error,
	 *																	recv overwrite
	 */
	outb (dev->resources.port + NE_P0_IMR,
			NE_IMR_PRXE | NE_IMR_PTXE | NE_IMR_RXEE | NE_IMR_TXEE |
      NE_IMR_OVWE);

	/* program command register for page 1 */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_1 | NE_CR_STP);

	/* copy out our station address */
	for (i = 0; i < 6; i++)
		outb (dev->resources.port + NE_P1_PAR0 + i, nc->addr[i]);

	/* TODO: multicast? */

	/* set current page pointer to one page after the boundary pointer */
	nc->next_packet = nc->rec_page_start + 1;
	outb (dev->resources.port + NE_P1_CURR, nc->next_packet);

	/* program command registers for page 0 */
	outb (dev->resources.port + NE_P1_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STP);

	/* allow broadcasts */
	outb (dev->resources.port + NE_P0_RCR, NE_RCR_AB);

	/* get rid of the loopback mode */
	outb (dev->resources.port + NE_P0_TCR, 0);

	/* fire up the interface */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STA);
}

/*
 * This will stop the interface.
 */
void
ne_stop (struct DEVICE* dev) {
	int n = 5000;

	/* stop the entire interface and select page 0 registers */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STP);

	/* wait for the interface to reach stopped state */
	while (((inb (dev->resources.port + NE_P0_ISR) & NE_ISR_RST) == 0) && --n);
}

/*
 * This will reset the interface.
 */
void
ne_reset (struct DEVICE* dev) {
	ne_stop (dev);
	ne_init_nic (dev);
}

/*
 * This will copy a packet from the NIC to a pointer.
 */
int
ne_ring_copy (struct DEVICE* dev, uint16_t src, uint8_t* dst, uint16_t len) {
	NE_CONFIG* nc = (NE_CONFIG*)dev->data;
	int tmp_amount;

	/* does copy wrap to lower addr in ring buffer? */
	if (src + len > nc->mem_end) {
		/* yes. handle it */
		tmp_amount = nc->mem_end - src;
		ne_pio_readmem (dev, src, dst, tmp_amount);
		len -= tmp_amount;
		src = nc->mem_ring;
		dst += tmp_amount;
	}

	ne_pio_readmem (dev, src, dst, len);
	return src + len;
}

/*
 * This will fetch the packet into [ptr]. It will return 0 on failure or 1
 * on success.
 */
int
ne_get (struct DEVICE* dev, uint16_t src, uint16_t len, uint8_t* ptr) {
	src = ne_ring_copy (dev, src, ptr, len);
	return 1;
}

/*
 * This will retrieve the packet.
 */
void
ne_read (struct DEVICE* dev, int buf, int len) {
	struct NETPACKET* pkt = network_alloc_packet (dev);
	if (pkt == NULL) {
		kprintf ("%s: out of network buffers!\n", dev->name);
		return;
	}

	/* fetch the packet from the card */
	if (!ne_get (dev, buf, len, pkt->frame))
		/* this failed, return */
		return;

	/* don't pass the ethernet header */
	pkt->len = len - sizeof (ETHERNET_HEADER);

	/* update statistics */
	dev->rx_frames++; dev->rx_bytes += len;

	/* handle the packet! */
	network_queue_packet (pkt);
}

/*
 * This will handle receive interrupts.
 */
void
ne_rint (struct DEVICE* dev) {
	NE_CONFIG* nc = (NE_CONFIG*)dev->data;
	uint8_t boundary, current;
	uint16_t len;
	uint8_t nlen;
	uint8_t next_packet;
	uint16_t count;
	uint8_t packet_hdr[NE_RING_HDRSZ];
	int packet_ptr;

loop:
	/* set nic to page 1 registers to get 'current' pointer */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_1 | NE_CR_STA);

	/* get the current pointer */
	current = inb (dev->resources.port + NE_P1_CURR);
	if (nc->next_packet == current)
		return;

	/* set nic to page 0 registers to update boundary register */
	outb (dev->resources.port + NE_P1_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STA);

	do {
		/* get pointer to this buffer's header structure */
		packet_ptr = nc->mem_ring + ((nc->next_packet - nc->rec_page_start) << NE_PAGE_SHIFT);

		/* the bute count includes a 4 byte header that was added by the nic */
		ne_pio_readmem (dev, (uint16_t)packet_ptr, packet_hdr, sizeof (packet_hdr));

		next_packet = packet_hdr[NE_RING_NEXT_PACKET];
		len = count = packet_hdr[NE_RING_COUNT] +
									256 * packet_hdr[NE_RING_COUNT + 1];

		/* recalculate the length for buggy cards */
		if (next_packet >= nc->next_packet)
			nlen = (next_packet - nc->next_packet);
		else
			nlen = ((next_packet - nc->rec_page_start) +
				(nc->rec_page_stop - nc->next_packet));
		--nlen;
		if ((len & NE_PAGE_MASK) + sizeof (packet_hdr) > NE_PAGE_SIZE)
			--nlen;
		len = (len & NE_PAGE_MASK) | (nlen << NE_PAGE_SHIFT);
		if (len != count) {
			kprintf ("%s: length does no match next packet pointer\n", dev->name);
		}

		if (len <= MCLBYTES &&
			next_packet >= nc->rec_page_start &&
			next_packet < nc->rec_page_stop) {
			/* go get packet */
			ne_read (dev, packet_ptr + NE_RING_HDRSZ, len - NE_RING_HDRSZ);
		} else {
			/* ring pointers corrupted! */
			kprintf ("%s: memory corrupted - invalid packet length %u\n", dev->name, len);
			ne_reset (dev);
		}

		/* update the next packet pointer */
		nc->next_packet = next_packet;

		/* update nic boundary pointer */
		boundary = nc->next_packet - 1;
		if (boundary < nc->rec_page_start)
			boundary = nc->rec_page_stop - 1;
		outb (dev->resources.port + NE_P0_BNRY, boundary);
	} while (nc->next_packet != current);

	goto loop;
}

/*
 * This will write [len] bytes of [dst] to the wire.
 */
int
ne_pio_write (struct DEVICE* dev, uint16_t len, uint16_t dst, uint8_t* data) {
	NE_CONFIG* nc = (NE_CONFIG*)dev->data;
	uint8_t maxwait = 120;

	/* select page 0 registers */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STA);

	/* reset remote DMA complete flag */
	outb (dev->resources.port + NE_P0_ISR, NE_ISR_RDC);

	/* set up DMA byte count */
	outb (dev->resources.port + NE_P0_RBCR0, (len & 0xff));
	outb (dev->resources.port + NE_P0_RBCR1, (len >> 8));

	/* set up destination address in NIC memory */
	outb (dev->resources.port + NE_P0_RSAR0, (dst & 0xff));
	outb (dev->resources.port + NE_P0_RSAR1, (dst >> 8));

	/* set remote DMA write */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD1 | NE_CR_PAGE_0 | NE_CR_STA);

	/* transfer the data into NIC memory */
	if (!nc->isa16bit) {
		kprintf ("%s: TODO: 8 bit writes\n", dev->name);
		return 0;
	} else {
		/* NE2000's are trickier */
		uint16_t savebyte;
#if 0
		uint32_t i;
		uint16_t d;
#endif

#if 0
		/* 16 bit */
		for (i = 0; i < (len & 0xffff); i += 2) {
			d = (uint16_t)(*(data + i)) | (uint16_t)((*(data + i + 1)) << 8);
			outw (dev->resources.port + 0x10, d);
		}
#endif
		outsw (dev->resources.port + 0x10, data, (len & 0xffff) >> 1);

		if (len & 1) {
			savebyte = ((uint8_t)*(data + (len & 0xffff)) << 8);
#if 0
			/* FIXME!!! this halts the system! */
			outw (dev->resources.port + 0x10, savebyte);
#endif
		}
	}

	/* wait for remote DMA complete */
	while (((inb (dev->resources.port + NE_P0_ISR) & NE_ISR_RDC) != NE_ISR_RDC) &&
				(--maxwait));
	if (!maxwait) {
		/*kprintf ("%s: remote DMA failed to complete\n", dev->name);*/
		return 0;
	}

	return len;
}

/*
 * This will actually start transmission.
 */
void
ne_xmit (struct DEVICE* dev) {
	NE_CONFIG* nc = (NE_CONFIG*)dev->data;
	uint16_t len = nc->txb_len[nc->txb_next_tx];

	/* set page 0 register access */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STA);

	/* set TX buffer start page */
	outb (dev->resources.port + NE_P0_TPSR, nc->tx_page_start + nc->txb_next_tx * NE_TXBUF_SIZE);

	/* set TX length */
	outb (dev->resources.port + NE_P0_TBCR0, len);
	outb (dev->resources.port + NE_P0_TBCR1, len >> 8);

	/* set page 0, remote DMA complete, transmit packet, start */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_TXP | NE_CR_STA);

	/* point to next transmit buffer slot */
	nc->txb_next_tx++;
	if (nc->txb_next_tx == nc->txb_cnt)
		nc->txb_next_tx = 0;
}

/*
 * This will start output of [len] bytes of [data].
 */
void
ne_start (struct DEVICE* dev) {
	NE_CONFIG* nc = (NE_CONFIG*)dev->data;
	int buffer, len;
	struct NETPACKET* pkt;

	/* see if there is room to put another packet in the buffer */
	if (nc->txb_inuse == nc->txb_cnt)
		/* no room. leave */
		return;

	/*
	 * Fetch the next packet to send
	 *
	 * the IRQ will acknowledge when we are done so we can send more
	 *
	 */
	pkt = network_get_next_txbuf (dev);
	if (pkt == NULL)
		return;

	/* txb_new points to the next open buffer slot */
	buffer = nc->mem_start + ((nc->txb_new * NE_TXBUF_SIZE) << NE_PAGE_SHIFT);

	/* write it */
	len = ne_pio_write (dev, pkt->len + pkt->header_len, (uint16_t)buffer, (uint8_t*)pkt->frame);
	if (!len) {
		/* this failed. XXX: re-queue the packet and bail out */
		return;
	}

	if (len > 60)
		nc->txb_len[nc->txb_new] = len;
	else
		nc->txb_len[nc->txb_new] = 60;

	/* start! */
	ne_xmit (dev);

	/* point to next buffer slot */
	if (++nc->txb_new == nc->txb_cnt)
		nc->txb_new = 0;
	nc->txb_inuse++;

	/* update statistics */
	dev->tx_frames++; dev->tx_bytes += pkt->len;

	/* mark this packet as available */
	network_free_packet (pkt);
}

/*
 * This will handle incoming IRQ's. 
 */
void
ne_irq (struct DEVICE* dev) {
	NE_CONFIG* nc = (NE_CONFIG*)dev->data;
	uint8_t isr;

	/* set the nic to page 0 registers */
	outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STA);

	/* fetch the isr */
	isr = inb (dev->resources.port + NE_P0_ISR);
	if (!isr)
		return;

	/* handle them all */
	for (;;) {
		/* reset all bits that we are acknowledging */
		outb (dev->resources.port + NE_P0_ISR, isr);

		/* handle transmitter interupts first */
		if (isr & (NE_ISR_PTX | NE_ISR_TXE)) {
			/* read collisions */
			inb (dev->resources.port + NE_P0_NCR) /*& 0x0f*/;
			inb (dev->resources.port + NE_P0_TSR);
			if (isr & NE_ISR_TXE) {
				kprintf ("%s: transmitter error\n", dev->name);
			} else {
				/* done with the buffer */
				nc->txb_inuse--;

				/* transmit more data if needed */
				ne_start (dev);
			}
		}

		/* handle receiver interrupts */
		if (isr & (NE_ISR_PRX | NE_ISR_RXE | NE_ISR_OVW)) {
			/* overwrite warning? */
			if (isr & NE_ISR_OVW) {
				/* yes. reset the nic */
				kprintf ("%s: warning - receive ring buffer overrun\n", dev->name);
				ne_reset (dev);
			} else {
				/* receiver error */
				if (isr & NE_ISR_RXE) {
					kprintf ("%s: receive error 0x%x\n", dev->name, inb (dev->resources.port + NE_P0_RSR));
					ne_reset (dev);
				}
			}

			/* handle the packet retrieval */
			ne_rint (dev);
		}

		/* put the nic back in standard state: page 0, remote DMA complete, start */
		outb (dev->resources.port + NE_P0_CR, NE_CR_RD2 | NE_CR_PAGE_0 | NE_CR_STA);

		/* if the counters overflow, read them to reset them */
		if (isr & NE_ISR_CNT) {
				(void)inb (dev->resources.port + NE_P0_CNTR0);
				(void)inb (dev->resources.port + NE_P0_CNTR1);
				(void)inb (dev->resources.port + NE_P0_CNTR2);
		}

		/* fetch the isr */
		isr = inb (dev->resources.port + NE_P0_ISR);
		if (!isr)
			break;
	}
}

/*
 * This will initialize the NE card.
 */
int
ne_init (char* name, struct DEVICE_RESOURCES* res) {
	struct DEVICE rdev;
	struct DEVICE* dev;
	NE_CONFIG* edata = (NE_CONFIG*)kmalloc (NULL, sizeof (NE_CONFIG), 0);
	int i;

	/* do we have resources? */
	if ((res->port == 0) || (res->irq == 0)) {
		/* no. complain */
		kprintf ("%s: port and irq must be supplied\n", name);
		return 0;
	}

	/* build the device */
	kmemset (&rdev, 0, sizeof (struct DEVICE));
	rdev.name = name;
	rdev.data = edata;
	rdev.xmit = ne_start;
	rdev.addr_len = ETHER_ADDR_LEN;
	kmemcpy (&rdev.resources, res, sizeof (struct DEVICE_RESOURCES));

	/* do we have a device here? */
	if (modne_probe (&rdev) < 0) {
		/* no. bail out */
		kfree (edata);
		kprintf ("%s: NE2000 card not found at io 0x%x irq 0x%x\n", name, res->port, res->irq);
		return 0;
	}

	/* register the device */
	dev = device_register (&rdev);
	if (dev == NULL) {
		/* this failed. complain */
		kprintf ("%s: unable to register device!\n", name);
		return 0;
	}

	/* fill out our device-specific data */
	dev->data = edata;
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		dev->ether.hw_addr[i] = edata->addr[i];

	/* go! */
	ne_init_nic (dev);

	/* wait for IRQ's */
	if (!irq_register (res->irq, &ne_irq, dev)) {
		/* this failed. complain */
		kprintf ("%s: unable to register irq\n", name);
		return 0;
	}

	/* all done */
	return 1;
}

/* vim:set ts=2 sw=2: */
