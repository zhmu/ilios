/*
 * ep.c - ILIOS 3COM Etherlink III Driver
 * (c) 2002 Rink Springer, BSD licensed
 *
 * This code is based on OpenBSD's sys/dev/ic/elink3.c, (c) 1996, 1997
 * Jonathan Stone, (c) 1994 Herb Peyerl
 *
 * This module handles all 3COM Etherlink III device stuff.
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
#include "ep.h"

struct EP_CARD {
	uint16_t	iobase;
	uint8_t		irq;
};

/* XXX */
extern void delay (int i);

#define ep_reset ep_init

/*
 * This will fetch register offsets.
 */
int
ep_w1_reg (struct DEVICE* dev, int reg) {
	struct EP_CONFIG* ep_config = (struct EP_CONFIG*)dev->data;

	switch (ep_config->ep_chipset) {
		case ELINK_CHIPSET_CORKSCREW:
			return (reg + 0x10);
		case ELINK_CHIPSET_ROADRUNNER:
			switch (reg) {
				case ELINK_W1_FREE_TX:
				case ELINK_W1_RUNNER_RDCTL:
				case ELINK_W1_RUNNER_WRCTL:
					return reg;
			}
			return (reg + 0x10);
	}

	return reg;
}

/*
 * This will read the word at offset [offset] of the selected Etherlink card,
 * using the ELINK_ID_PORT stuff.
 */
uint16_t
epreadeeprom (int offset) {
	int i;
	uint16_t data = 0;

	outb (ELINK_ID_PORT, 0x80 + offset);
	delay (1000);	
	for (i = 0; i < 16; i++)
		data = (data << 1) | (inw (ELINK_ID_PORT) & 1);
	return data;
}

/*
 * This will check whether the EEPROM is busy. It will return zero if idle
 * and non-zero if busy.
 */
int
ep_busy_eeprom (struct DEVICE* dev) {
	struct EP_CONFIG* ep_config = (struct EP_CONFIG*)dev->data;
	uint16_t busybit, eecmd;
	int i = 100, j;

	if (ep_config->ep_chipset == ELINK_CHIPSET_CORKSCREW) {
		eecmd = CORK_ASIC_EEPROM_COMMAND;
		busybit = CORK_EEPROM_BUSY;
	} else {
		eecmd = ELINK_W0_EEPROM_COMMAND;
		busybit = EEPROM_BUSY;
	}

	j = 0;
	while (i--) {
		j = inw (dev->resources.port + eecmd);
		if (j & busybit)
			delay (100000);
		else
			break;
	}

	if (i == 0) {
		kprintf ("%s: EEPROM failed to come ready\n", dev->name);
		return 1;
	}

	if ((ep_config->ep_chipset != ELINK_CHIPSET_CORKSCREW) &&
	    (j & EEPROM_TST_MODE) != 0) {
		kprintf ("%s: erase pencil mark\n", dev->name);
		return 1;
	}

	return 0;
}

/*
 * This will read the word at offset [offset].
 */
uint16_t
ep_read_eeprom (struct DEVICE* dev, int offset) {
	uint16_t eecmd, eedata, readcmd;
	struct EP_CONFIG* ep_config = (struct EP_CONFIG*)dev->data;

	if (ep_config->ep_chipset == ELINK_CHIPSET_CORKSCREW) {
		eecmd = CORK_ASIC_EEPROM_COMMAND;
		eedata = CORK_ASIC_EEPROM_DATA;
	} else {
		eecmd = ELINK_W0_EEPROM_COMMAND;
		eedata = ELINK_W0_EEPROM_DATA;
	}
	if (ep_config->ep_chipset == ELINK_CHIPSET_ROADRUNNER)
		readcmd = READ_EEPROM_RR;
	else
		readcmd = READ_EEPROM;

	if (ep_busy_eeprom (dev))
		return 0;

	outw (dev->resources.port + eecmd, readcmd | offset);

	if (ep_busy_eeprom (dev))
		return 0; 

	return inw (dev->resources.port + eedata);
}

/*
 * This will display the configuration.
 */
void
ep_showconfig (struct DEVICE* dev) {
	uint32_t config0, config1;
	const char* onboard_ram_config[] = { "5:3", "3:1", "1:1", "3:5" };
	int ram_size, ram_width, ram_speed, rom_size, ram_split;
	struct EP_CONFIG* ep_config = (struct EP_CONFIG*)dev->data;

	GO_WINDOW(3);
	config0 = (uint32_t)inw (dev->resources.port + ELINK_W3_INTERNAL_CONFIG);
	config1 = (uint32_t)inw (dev->resources.port + ELINK_W3_INTERNAL_CONFIG + 2);
	GO_WINDOW(0);

	ram_size  = (config0 & CONFIG_RAMSIZE ) >> CONFIG_RAMSIZE_SHIFT ;
	ram_width = (config0 & CONFIG_RAMWIDTH) >> CONFIG_RAMWIDTH_SHIFT;
	ram_speed = (config0 & CONFIG_RAMSPEED) >> CONFIG_RAMSPEED_SHIFT;
	rom_size  = (config0 & CONFIG_ROMSIZE ) >> CONFIG_ROMSIZE_SHIFT ;
	ram_split = (config1 & CONFIG_RAMSPLIT) >> CONFIG_RAMSPLIT_SHIFT;

	kprintf ("%s: %uKB %s-wide FIFO, %s Rx:Tx split,",
		dev->name, 8 << ram_size, (ram_width) ? "word" : "byte",
		onboard_ram_config[ram_split]);
	if (ep_config->ep_connectors & ELINK_W0_CC_BNC)
		kprintf (" BNC");
	if (ep_config->ep_connectors & ELINK_W0_CC_UTP)
		kprintf (" UTP");
	if (ep_config->ep_connectors & ELINK_W0_CC_AUI)
		kprintf (" AUI");
	kprintf ("\n");
}

/* Wait for a pending reset to complete
 */
void
ep_finish_reset (struct DEVICE* dev) {
	int i;

	for (i = 0; i < 10000; i++) {
		if ((inw (dev->resources.port + ELINK_STATUS) & COMMAND_IN_PROGRESS) == 0)
			break;
		delay(10);
	}
}

/* Issues a reset command, and ensures it has completed.
 */
void
ep_reset_cmd (struct DEVICE* dev, int cmd, int arg) {
	outw (dev->resources.port + cmd, arg);
	ep_finish_reset (dev);
}

/* This will fetch the connectors the card has
 */
void
ep_get_media (struct DEVICE* dev) {
	struct EP_CONFIG* ep_config = (struct EP_CONFIG*)dev->data;
	uint16_t config;

	GO_WINDOW(0);
	ep_config->ep_connectors = 0;

	config = inw (dev->resources.port + ELINK_W0_CONFIG_CTRL);
	if (config & ELINK_W0_CC_AUI)
		ep_config->ep_connectors |= ELINK_W0_CC_AUI;
	if (config & ELINK_W0_CC_BNC)
		ep_config->ep_connectors |= ELINK_W0_CC_BNC;
	if (config & ELINK_W0_CC_UTP)
		ep_config->ep_connectors |= ELINK_W0_CC_UTP;
}

/*
 * This will configure a device.
 */
void
epconfig (struct DEVICE* dev) {
	int i;
	uint16_t x;
	struct EP_CONFIG* ep_config = (struct EP_CONFIG*)dev->data;

	/* ensure we are in window 0, so we can read the EEPROM */
	GO_WINDOW(0);

	/* fetch the ethernet address */
	for (i = 0; i < (ETHER_ADDR_LEN / 2); i++) {
		x = ep_read_eeprom (ep_config->dev, i);
		ep_config->dev->ether.hw_addr[(i << 1)]     = (x >> 8);
		ep_config->dev->ether.hw_addr[(i << 1) + 1] = x;
	}

	/* detect large packet support */
	outw (dev->resources.port + ELINK_COMMAND, SET_TX_AVAIL_THRESH | ELINK_LARGEWIN_PROBE);
	GO_WINDOW(5);
	i = inw (dev->resources.port + ELINK_W5_TX_AVAIL_THRESH);
	GO_WINDOW(1);
	switch (i) {
		case ELINK_LARGEWIN_PROBE:
		case (ELINK_LARGEWIN_PROBE & ELINK_LARGEWIN_MASK):
			ep_config->ep_pktlenshift = 0;
			break;
		case (ELINK_LARGEWIN_PROBE << 2):
			ep_config->ep_pktlenshift = 2;
			break;
		default:
			kprintf ("%s: weird value detected when detecting large packet support\n");
			ep_config->ep_pktlenshift = 0;
	}

	/* ensure TX-available interrupts are enabled */
	outw (dev->resources.port + ELINK_COMMAND, SET_TX_AVAIL_THRESH | (1600 >> ep_config->ep_pktlenshift));

	/* fetch the media config */
	ep_get_media (dev);

	/* display some info */
	ep_showconfig (dev);

	/* Window 1 is operating mode */
	GO_WINDOW(1);
	ep_config->tx_start_thresh = 20;

	ep_reset_cmd (dev, ELINK_COMMAND, RX_RESET);
	ep_reset_cmd (dev, ELINK_COMMAND, TX_RESET);
}

void
ep_discard_rxtop (struct DEVICE* dev) {
	int i;

	outw (dev->resources.port + ELINK_COMMAND, RX_DISCARD_TOP_PACK);
	for (i = 0; i < 8000; i++)
		if ((inw (dev->resources.port + ELINK_STATUS) & COMMAND_IN_PROGRESS) == 0)
			return;
	ep_finish_reset (dev);
}

/* Shut the interface down
 */
void
ep_stop (struct DEVICE* dev) {
	struct EP_CONFIG* ep_config = (struct EP_CONFIG*)dev->data;

	if (ep_config->ep_chipset == ELINK_CHIPSET_ROADRUNNER) {
		/* clear the FIFO buffer count */
		GO_WINDOW(1);
		outw (dev->resources.port + ELINK_W1_RUNNER_WRCTL, 0);
		outw (dev->resources.port + ELINK_W1_RUNNER_RDCTL, 0);
	}

	outw (dev->resources.port + ELINK_COMMAND, RX_DISABLE);
	ep_discard_rxtop (dev);

	outw (dev->resources.port + ELINK_COMMAND, TX_DISABLE);
	outw (dev->resources.port + ELINK_COMMAND, STOP_TRANSCEIVER);

	ep_reset_cmd (dev, ELINK_COMMAND, RX_RESET);
	ep_reset_cmd (dev, ELINK_COMMAND, TX_RESET);

	outw (dev->resources.port + ELINK_COMMAND, ACK_INTR | INTR_LATCH);
	outw (dev->resources.port + ELINK_COMMAND, SET_RD_0_MASK);
	outw (dev->resources.port + ELINK_COMMAND, SET_INTR_MASK);
	outw (dev->resources.port + ELINK_COMMAND, SET_RX_FILTER);
}

/* Set the card to use specific media
 */
void
ep_setmedia (struct DEVICE* dev) {
	int w0_addr_cfg, i;

	/* turn everything off */
	GO_WINDOW(0);
	outw (dev->resources.port + ELINK_COMMAND, STOP_TRANSCEIVER);
	GO_WINDOW(4);
	outw (dev->resources.port + ELINK_W4_MEDIA_TYPE, 0);
	GO_WINDOW(0);

	/* turn on the selected media/transceiver */
	switch (2) {
		case 1: /* 10baseT (UTP) */
		        GO_WINDOW(4);
		        outw (dev->resources.port + ELINK_W4_MEDIA_TYPE, JABBER_GUARD_ENABLE | LINKBEAT_ENABLE);
						i = 0;
		        break;
		case 2: /* 10base2 (BNC) */
		        outw (dev->resources.port + ELINK_COMMAND, START_TRANSCEIVER);
		        delay(1000);
						i = 3;
		        break;
		case 3: /* 10base5 (AUI) */
						i = 1;
		        break;
	}

	/* tell the chip which port to use */
	GO_WINDOW(0);
	w0_addr_cfg = inw (dev->resources.port + ELINK_W0_ADDRESS_CFG) & 0x3fff;
	outw (dev->resources.port + ELINK_W0_ADDRESS_CFG, w0_addr_cfg | (i << 14));
	delay(1000);

	GO_WINDOW(1);
}

/* This will bring the device up
 */
void
ep_init (struct DEVICE* dev) {
	struct EP_CONFIG* ep_config = (struct EP_CONFIG*)dev->data;
	int i;

	/* complete any pending resets */
	ep_finish_reset (dev);

	/* cancel any pending I/O */
	ep_stop (dev);

	GO_WINDOW(0);
	outw (dev->resources.port + ELINK_W0_CONFIG_CTRL, 0);
	outw (dev->resources.port + ELINK_W0_CONFIG_CTRL, ENABLE_DRQ_IRQ);
	GO_WINDOW(2);

	/* reload the address */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		outb (dev->resources.port + ELINK_W2_ADDR_0 + i,
			dev->ether.hw_addr[i]);

	/* reset the station-address receive filter */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		outb (dev->resources.port + ELINK_W2_RECVMASK_0 + i, 0);

	ep_reset_cmd (dev, ELINK_COMMAND, RX_RESET);
	ep_reset_cmd (dev, ELINK_COMMAND, TX_RESET);

	GO_WINDOW(1);
	for (i = 0; i < 31; i++)
		inb (dev->resources.port + ep_w1_reg (dev, ELINK_W1_TX_STATUS));

	/* set threshold for tx-space available interrupts */
	outw (dev->resources.port + ELINK_COMMAND,
		SET_TX_AVAIL_THRESH | (1600 >> ep_config->ep_pktlenshift));

	/* enable interrupts */
	outw (dev->resources.port + ELINK_COMMAND,
		SET_RD_0_MASK | WATCHED_INTERRUPTS);
	outw (dev->resources.port + ELINK_COMMAND,
		SET_INTR_MASK | WATCHED_INTERRUPTS);

	/* get rid of stray interrupts */
	outw (dev->resources.port + ELINK_COMMAND, ACK_INTR | 0xff);

	/* set multicast receive filter */
	GO_WINDOW(1);
	outw (dev->resources.port + ELINK_COMMAND,
		SET_RX_FILTER | FIL_INDIVIDUAL | FIL_BRDCST | FIL_PROMISC);

	/* set media */
	ep_setmedia (dev);

	outw (dev->resources.port + ELINK_COMMAND, RX_ENABLE);
	outw (dev->resources.port + ELINK_COMMAND, TX_ENABLE);
}

/* This will fetch [len] bytes from [dev] inside a cute netpacket.
 */
struct NETPACKET*
ep_get (struct DEVICE* dev, int len) {
	struct NETPACKET* pkt = network_alloc_packet (dev);
	int remaining = len;
	/*uint16_t w;*/
	addr_t offset = 0;
	int rxreg/*, i*/;

	if (pkt == NULL) {
		kprintf ("%s: out of network buffers!\n", dev->name);
		return NULL;
	}

	rxreg = ep_w1_reg (dev, ELINK_W1_RX_PIO_RD_1);

	if ((remaining > 1) && (offset & 1)) {
		pkt->frame[offset] = inb (dev->resources.port + rxreg);
		remaining--;
		offset++;
	}
	if (remaining > 1) {
#if 0
		for (i = 0; i < remaining >> 1; i++) {
			w = inw (dev->resources.port + rxreg);
			pkt->frame[offset++] = (w & 0xff);
			pkt->frame[offset++] = (w >> 8);
		}
#else
		insw (dev->resources.port + rxreg, pkt->frame + offset, (remaining >> 1));
#endif
	}
	if (remaining & 1) {
		pkt->frame[offset] = inb (dev->resources.port + rxreg);
	}

	ep_discard_rxtop (dev);

	/* all done */
	return pkt;
}

int
ep_status (struct DEVICE* dev) {
	uint16_t fifost;

	GO_WINDOW(4);
	fifost = inw (dev->resources.port + ELINK_W4_FIFO_DIAG);
	GO_WINDOW(1);

	if (fifost & FIFOS_RX_UNDERRUN) {
		ep_reset (dev);
		return 0;
	}

	if (fifost & FIFOS_RX_STATUS_OVERRUN)
		return 1;

	if (fifost & FIFOS_TX_OVERRUN) {
		ep_reset (dev);
		return 0;
	}

	return 0;
}

/*
 * Called on incoming data
 */
void
ep_read (struct DEVICE* dev) {
	int len;
	struct NETPACKET* pkt;

	len = inw (dev->resources.port + ep_w1_reg (dev, ELINK_W1_RX_STATUS));

again:
	if (len & ERR_INCOMPLETE)
		return;

	if (len & ERR_RX)
		goto abort;

	/* lower 11 bits = RX bytes */
	len &= RX_BYTES_MASK;

	/* fetch */
	pkt = ep_get (dev, len);
	if (pkt == NULL)
		goto abort;

#if 0
	kprintf ("%s: got packet [", dev->name);
	for (vv = 0; vv < len; vv++)
		kprintf ("%x ", (uint8_t)(pkt->frame[vv]));
	kprintf ("]\n");
#endif

	/* update statistics */
	dev->rx_frames++; dev->rx_bytes += len;

	/* handle the input */
	pkt->len = len - sizeof (ETHERNET_HEADER); 
	network_queue_packet (pkt);

	/* fix overflow */
	if (ep_status (dev)) {
		len = inw (dev->resources.port + ep_w1_reg (dev, ELINK_W1_RX_STATUS));
		if (len & ERR_INCOMPLETE) {
			ep_reset (dev);
			return;
		}
		goto again;
	}

	return;

abort:
	ep_discard_rxtop (dev);
}

/*
 * Start output on the interface
 */
void
ep_start (struct DEVICE* dev) {
	struct NETPACKET* pkt;
	int pad, len, txreg, i;
	struct EP_CONFIG* ep_config = (struct EP_CONFIG*)dev->data;
	uint16_t d, status;
	uint8_t b;
	uint8_t* data;

startagain:
	/* Fetch the next packet to send */
	pkt = network_get_next_txbuf (dev);
	if (pkt == NULL)
		return;

	len = pkt->len + pkt->header_len;
	pad = (4 - len) & 3;

	/* drop packets which are too large */
	if (len + pad > NETWORK_MAX_PACKET_LEN) {
		network_free_packet (pkt);
		goto readcheck;
	}

	
	if (inw (dev->resources.port + ep_w1_reg (dev, ELINK_W1_FREE_TX)) < (len + pad + 4)) {
		outw (dev->resources.port + ELINK_COMMAND, SET_TX_AVAIL_THRESH | ((len + pad + 4) >> ep_config->ep_pktlenshift));
		return;
	} else {
		outw (dev->resources.port + ELINK_COMMAND, SET_TX_AVAIL_THRESH | ELINK_THRESH_DISABLE);
	}

	/* XXX: remove packet only now */
	outw (dev->resources.port + ELINK_COMMAND, SET_TX_AVAIL_THRESH |
		((len / 4 + ep_config->tx_start_thresh)));

	txreg = ep_w1_reg (dev, ELINK_W1_TX_PIO_WR_1);
	/* XXX: fifo */

	/* write the length */
	outw (dev->resources.port + txreg, len);
	outw (dev->resources.port + txreg, 0xffff);

	/* feed the nic the data */
	data = (uint8_t*)pkt->frame;
	for (i = 0; i < (len & 0xffff); i += 2) {
		d = (uint16_t)(*(data + i)) | (uint16_t)((*(data + i + 1)) << 8);
		outw (dev->resources.port + txreg, d);
	}
	if (len & 1) {
		b = ((uint8_t)*(data + (len & 0xffff)) << 8);
		outb (dev->resources.port + txreg, b);
	}
	while (pad--)
		outb (dev->resources.port + txreg, 0);

	/* update statistics */
	dev->tx_frames++; dev->tx_bytes += pkt->len;

	/* free the buffer */
	network_free_packet (pkt);

readcheck:
	if ((inw (dev->resources.port + ep_w1_reg (dev, ELINK_W1_RX_STATUS)) & ERR_INCOMPLETE) == 0) {
		/* we received a complete packet */
		status = inw (dev->resources.port + ELINK_STATUS);

		if ((status & INTR_LATCH) == 0) {
			/* got a packet without an IRQ [huh?]. handle it */
			ep_read (dev);
		} else {
			return;
		}
	} else {
		/* check if we are stuck */
		if (ep_status (dev))
			ep_reset (dev);
	}

	goto startagain;
}

void
ep_txstat (struct DEVICE* dev) {
	struct EP_CONFIG* ep_config = (struct EP_CONFIG*)dev->data;
	int i;

	while ((i = inb (dev->resources.port + ep_w1_reg (dev, ELINK_W1_TX_STATUS))) & TXS_COMPLETE) {
		outb (dev->resources.port + ep_w1_reg (dev, ELINK_W1_TX_STATUS), 0);

		if (i & TXS_JABBER)
			ep_reset (dev);
		else if (i & TXS_UNDERRUN) {
			if (ep_config->tx_succ_ok < 100)
				ep_config->tx_start_thresh = 1600 < ep_config->tx_start_thresh + 20 ? 1600 : ep_config->tx_start_thresh + 20;
			ep_reset (dev);
		} else if (i & TXS_MAX_COLLISION) {
			outw (dev->resources.port + ELINK_COMMAND, TX_ENABLE);
		} else
			ep_config->tx_succ_ok = (ep_config->tx_succ_ok + 1) & 127;
	}
}

/*
 * Interrupt service routine.
 */
void
ep_irq (struct DEVICE* dev) {
	uint16_t status;

	for (;;) {
		status = inw (dev->resources.port + ELINK_STATUS);

		if ((status & WATCHED_INTERRUPTS) == 0)
			if ((status & INTR_LATCH) == 0)
				break;

		/* acknowledge any interrupts */
		outw (dev->resources.port + ELINK_COMMAND, ACK_INTR |
			(status & (INTR_LATCH | ALL_INTERRUPTS)));

		if (status & RX_COMPLETE)
			ep_read (dev);
		if (status & TX_AVAIL)
			ep_start (dev);
		if (status & CARD_FAILURE) {
			kprintf ("%s: adapter failure (%x)\n", dev->name, status);
			ep_init (dev);
		}
		if (status & TX_COMPLETE) {
			ep_txstat (dev);
			ep_start (dev);
		}
	}
}

/*
 * This will attach an Etherlink card [no] at [card] using chipset [chipset].
 */
void
ep_attach (int no, struct EP_CARD* card, uint16_t chipset) {
	struct DEVICE dev;
	struct EP_CONFIG* ep_config = (struct EP_CONFIG*)kmalloc (NULL, sizeof (struct EP_CONFIG), 0);

	/* build the device */	
	kmemset (&dev, 0, sizeof (struct DEVICE));
	dev.name = "ep?";
	dev.name[2] = no + 0x30;
	dev.addr_len = ETHER_ADDR_LEN;
	dev.data = ep_config;
	dev.xmit = ep_start;
	dev.resources.port = card->iobase;
	dev.resources.irq = card->irq;
	ep_config->dev = device_register (&dev);
	ep_config->ep_chipset = chipset;

	/* hook the interrupt */
	if (!irq_register (card->irq, &ep_irq, ep_config->dev)) {
		kprintf ("%s: unable to register irq\n", ep_config->dev->name);
	}

	/* configure the nic */
	epconfig (ep_config->dev);
	ep_init (ep_config->dev);
}

/*
 * This will probe for all Etherlink cards in the system. 
 */
void
ep_probe() {
	int slot, i, j;
	uint8_t c, p = ELINK_509_POLY;
	uint16_t vendor, model, iobase, cksum, irq;
	uint8_t nepcards = 0;
	struct EP_CARD epcards[MAX_EP_CARDS];

	/* reset the cards */
	outb (ELINK_ID_PORT, ELINK_RESET);
	outb (ELINK_ID_PORT, 0);

	/* probe all cards */
	for (slot = 0; slot < MAX_EP_CARDS; slot++) {
		/* send the ID sequence */
		c = 0xff;
		for (j = 255; j; j--) {
			outb (ELINK_ID_PORT, c);
			if (c & 0x80) {
				c <<= 1; c ^= p;
			} else
				c <<= 1;
		}

		/* untag all adapters so they will talk to us */
		if (slot == 0)
			outb (ELINK_ID_PORT, TAG_ADAPTER + 0);

		/* fetch the vendor */
		vendor = ntohs (epreadeeprom (EEPROM_MFG_ID));
		if (vendor != MFG_ID)
			continue;

		/* fetch the model */
		model = ntohs (epreadeeprom (EEPROM_PROD_ID));
		if ((model & 0xfff0) != PROD_ID_3C509) {
			kprintf ("ep: ignoring model %x\n", model);
			continue;
		}

		/* fetch the io base and irq */
		iobase = epreadeeprom (EEPROM_ADDR_CFG);
		iobase = (iobase & 0x1f) * 0x10 + 0x200;
		irq = epreadeeprom (EEPROM_RESOURCE_CFG);
		irq >>= 12;

		/* fetch the secondary checksum */
		cksum = epreadeeprom (EEPROM_CHECKSUM_EL3) & 0xff;
		for (i = EEPROM_CONFIG_HIGH; i < EEPROM_CHECKSUM_EL3; i++)
			cksum ^= epreadeeprom (i);
		cksum = (cksum & 0xff) ^ ((cksum >> 8) & 0xff);

		/* stop the card from responding */
		outb (ELINK_ID_PORT, TAG_ADAPTER + 1);

		/* got a correct checksum ? */
		if (cksum != 0) {
			/* no. complain */
			kprintf ("ep: checksum mismatch\n");
			continue;
		}

		/* activate the configuration */
		outb (ELINK_ID_PORT, ACTIVATE_ADAPTER_TO_CONFIG);

		/* add the card */
		epcards[nepcards].iobase = iobase;
		epcards[nepcards].irq = (irq == 2) ? 9 : irq;
		nepcards++;
	}

	/* attach all cards one by one */
	for (i = 0; i < nepcards; i++)
		ep_attach (i, &epcards[i], ELINK_CHIPSET_3C509);
}

/* vim:set ts=2 sw=2: */
