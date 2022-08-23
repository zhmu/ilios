/*
 * rtl8139.c - ILIOS RealTek 8139 Card Driver
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 * This code is based on OpenBSD's sys/dev/pci/if_rl_pci.c and
 * sys/dev/ic/rtl81x9.c, both (c) 1997, 1998 Bill Paul <wpaul@ctr.columbia.edu>
 *
 * This module handles all RTL8139 device stuff.
 *
 */
#include <sys/device.h>
#include <sys/irq.h>
#include <sys/network.h>
#include <sys/kmalloc.h>
#include <sys/types.h>
#include <lib/lib.h>
#include <md/config.h>
#include <md/interrupts.h>
#include <md/timer.h>
#include <md/pio.h>
#include "rtl8139.h"

#define xRTL_DEBUG_RX

void rl_init(struct DEVICE* dev);
void rl_start (struct DEVICE* dev);

/*
 * Send a read command and address to the EEPROM and check for ACK.
 */
void
rl_eeprom_putbyte (struct DEVICE* dev, uint32_t addr, uint32_t addr_len) {
	register int d, i;

	d = (RL_EECMD_READ << addr_len) | addr;

	/* feed in each bit and strobe the clock */
	for (i = RL_EECMD_LEN + addr_len; i; i--) {
		if (d & (1 << (i - 1)))
			EE_SET (RL_EE_DATAIN);
		else
			EE_CLR (RL_EE_DATAIN);

		arch_delay (100);
		EE_SET (RL_EE_CLK);
		arch_delay (150);
		EE_CLR (RL_EE_CLK);
		arch_delay (100);
	}
}

/*
 * Read a word of data stored in the EEPROM at address [addr]
 */
void
rl_eeprom_getword (struct DEVICE* dev, uint32_t addr, uint32_t addr_len, uint16_t* dest) {
	register int i;
	uint16_t word = 0;

	/* enter EEPROM access mode */
	CSR_WRITE_1 (dev, RL_EECMD, RL_EEMODE_PROGRAM | RL_EE_SEL);

	/* send address of word we want to read */
	rl_eeprom_putbyte (dev, addr, addr_len);

	CSR_WRITE_1 (dev, RL_EECMD, RL_EEMODE_PROGRAM | RL_EE_SEL);

	/* start reading bits from the EEPROM */
	for (i = 16; i > 0; i--) {
		EE_SET (RL_EE_CLK);
		arch_delay (100);
		if (CSR_READ_1 (dev, RL_EECMD) & RL_EE_DATAOUT)
			word |= 1 << (i - 1);
		EE_CLR (RL_EE_CLK);
		arch_delay (100);
	}

	/* turn off EEPROM access mode */
	CSR_WRITE_1 (dev, RL_EECMD, RL_EEMODE_OFF);
	*dest = word;
}

/*
 * read a sequence of words from the EEPROM
 */
void
rl_read_eeprom (struct DEVICE* dev, void* dest, uint32_t off, uint32_t addr_len, uint32_t cnt, uint32_t swap) {
	int i;
	uint16_t word = 0, *ptr;

	for (i = 0; i < cnt; i++) {
		rl_eeprom_getword (dev, off + i, addr_len, &word);
		ptr = (uint16_t*)(dest + (i * 2));
		if (swap)
			*ptr = (word >> 8) | ((word & 0xff) << 8);
		else
			*ptr = word;
	}
}

/* 
 * Sync the PHY's by setting data bit and strobing the clock 32 times.
 */
void
rl_mii_sync (struct DEVICE* dev) {
	register int i;

	MII_SET (RL_MII_DIR | RL_MII_DATAOUT);

	for (i = 0; i < 32; i++) {
		MII_SET (RL_MII_CLK);
		arch_delay (1);
		MII_SET (RL_MII_CLK);
		arch_delay (1);
	}
}

/*
 * Clock a series of bits through the MII
 */
void
rl_mii_send (struct DEVICE* dev, uint32_t bits, uint32_t cnt) {
	uint32_t i;
	
	MII_CLR (RL_MII_CLK);

	for (i = (0x1 << (cnt - 1)); i; i >>= 1) {
		if (bits & i)
			MII_SET (RL_MII_DATAOUT);
		else
			MII_CLR (RL_MII_DATAOUT);
		arch_delay (1);
		MII_CLR (RL_MII_CLK);
		arch_delay (1);
		MII_CLR (RL_MII_CLK);
	}
}

/*
 * Read an PHY register through the MII
 */
int
rl_mii_readreg (struct DEVICE* dev, struct RL_MII_FRAME* frame) {
	uint16_t ack;
	int i;

	/* set up frame for RX */
	frame->mii_stdelim = RL_MII_STARTDELIM;
	frame->mii_opcode = RL_MII_READOP;
	frame->mii_turnaround = 0;
	frame->mii_data = 0;

	CSR_WRITE_2 (dev, RL_MII, 0);

	/* turn on data xmit */
	MII_SET (RL_MII_DIR);
	rl_mii_sync (dev);

	/* send command/address info */
	rl_mii_send (dev, frame->mii_stdelim, 2);
	rl_mii_send (dev, frame->mii_opcode, 2);
	rl_mii_send (dev, frame->mii_phyaddr, 2);
	rl_mii_send (dev, frame->mii_regaddr, 2);

	/* idle bit */
	MII_CLR (RL_MII_CLK | RL_MII_DATAOUT);
	arch_delay (1);
	MII_SET (RL_MII_CLK);
	arch_delay (1);

	/* turn off xmit */
	MII_CLR (RL_MII_DIR);

	/* check for ack */
	MII_CLR (RL_MII_CLK);
	arch_delay (1);
	MII_SET (RL_MII_CLK);
	arch_delay (1);

	ack = CSR_READ_2 (dev, RL_MII) & RL_MII_DATAIN;

	/* try to read data bits. if the ack failed, we still need to clock through
	 * 16 cycles to keep the MII in sync */
	if (ack) {
		for (i = 0; i < 16; i++) {
			MII_CLR (RL_MII_CLK);
			arch_delay (1);
			MII_CLR (RL_MII_CLK);
			arch_delay (1);
		}
		goto fail;
	}

	for (i = 0x8000; i; i >>= 1) {
		MII_CLR (RL_MII_CLK);
		arch_delay (1);
		if (!ack) {
			if (CSR_READ_2 (dev, RL_MII) & RL_MII_DATAIN)
				frame->mii_data |= i;
			arch_delay (1);
		}
		MII_SET (RL_MII_CLK);
		arch_delay (1);
	}

fail:
	MII_CLR (RL_MII_CLK);
	arch_delay (1);
	MII_CLR (RL_MII_CLK);
	arch_delay (1);

	return (ack) ? 1 : 0;
}

/*
 * write to a PHY register through the MII
 */
int
rl_mii_writereg (struct DEVICE* dev, struct RL_MII_FRAME* frame) {
	/* set up frame for RX */
	frame->mii_stdelim = RL_MII_STARTDELIM;
	frame->mii_opcode = RL_MII_WRITEOP;
	frame->mii_turnaround = RL_MII_TURNAROUND;

	/* turn on data output */
	MII_SET (RL_MII_DIR);
	rl_mii_sync (dev);

	rl_mii_send (dev, frame->mii_stdelim, 2);
	rl_mii_send (dev, frame->mii_opcode, 2);
	rl_mii_send (dev, frame->mii_phyaddr, 5);
	rl_mii_send (dev, frame->mii_regaddr, 5);
	rl_mii_send (dev, frame->mii_turnaround, 2);
	rl_mii_send (dev, frame->mii_data, 16);

	/* idle bit */
	MII_CLR (RL_MII_CLK | RL_MII_DATAOUT);
	arch_delay (1);
	MII_SET (RL_MII_CLK);
	arch_delay (1);

	/* turn off xmit */
	MII_CLR (RL_MII_DIR);

	return 0;
}

/*
 * rl_reset (struct DEVICE* dev)
 *
 * This will reset the RTL8139.
 *
 */
void
rl_reset (struct DEVICE* dev) {
	register int i;

	CSR_WRITE_1 (dev, RL_COMMAND, RL_CMD_RESET);
	
	for (i = 0; i < RL_TIMEOUT; i++) {
		arch_delay (10);
		if (!(CSR_READ_1 (dev, RL_COMMAND) & RL_CMD_RESET))
			break;
	}
	if (i == RL_TIMEOUT)
		kprintf ("%s: reset never completed!\n", dev->name);
}

/*
 * Initialize the transmit descriptors.
 */
int
rl_list_tx_init (struct DEVICE* dev) {
	int i;
	struct RL_DATA* rld = (struct RL_DATA*)dev->data;

	for (i = 0; i < RL_TX_LIST_CNT; i++) {
		rld->tx_chain[i] = NULL;
		CSR_WRITE_4 (dev, RL_TXADDR0 + (i * sizeof (uint32_t)), 0);
	}

	rld->cur_tx = 0;
	rld->last_tx = 0;

	return 0;
}

void
rl_rxeof (struct DEVICE* dev) {
	int total_len = 0;
	uint32_t rx_stat, rxbufpos;
	int wrap = 0;
	uint16_t cur_rx, limit, rx_bytes = 0, max_bytes;
	struct RL_DATA* rld = (struct RL_DATA*)dev->data;
	struct NETPACKET* pkt;

	cur_rx = (CSR_READ_2 (dev, RL_CURRXADDR) + 16) % RL_RXBUFLEN;
	limit = CSR_READ_2 (dev, RL_CURRXBUF) % RL_RXBUFLEN;

	if (limit < cur_rx)
		max_bytes = (RL_RXBUFLEN - cur_rx) + limit;
	else
		max_bytes = limit - cur_rx;

	while ((CSR_READ_1 (dev, RL_COMMAND) & RL_CMD_EMPTY_RXBUF) == 0) {
		rxbufpos = rld->rx_buf + cur_rx;
		rx_stat = *(uint32_t*)rxbufpos;

		if ((uint16_t)(rx_stat >> 16) == RL_RXSTAT_UNFINISHED)
			break;

		if (!(rx_stat & RL_RXSTAT_RXOK)) {
			/* error! */
			rl_init (dev);
			return;
		}

		/* no errors, receive the header */
		total_len = rx_stat >> 16;
		rx_bytes += total_len + 4;

		/* discard the checksum */
		total_len -= ETHER_CRC_LEN;
		
		/* avoid reading more data than the chip has prepared for us */
		if (rx_bytes > max_bytes)
			break;

		rxbufpos = rld->rx_buf + ((cur_rx + sizeof (uint32_t)) % RL_RXBUFLEN);
		if (rxbufpos == (rld->rx_buf + RL_RXBUFLEN))
			rxbufpos = rld->rx_buf;

		wrap = (rld->rx_buf + RL_RXBUFLEN) - rxbufpos;
#ifdef RTL_DEBUG_RX
		kprintf ("cur_rx=%x,total_len=%x,rx_bytes=%x,maxbytes=%x,wrap=%x,rx_buf=%x,rxbufpos=%x\n", cur_rx, total_len, rx_bytes, max_bytes, wrap, rld->rx_buf, rxbufpos);
#endif

		/* fetch the packet */
		pkt = network_alloc_packet (dev);

		/* update statistics */
		dev->rx_frames++; dev->rx_bytes += total_len;

		/*
		 * NOTICE: the documentation on the receive operation plainly *SUCKS*, so
		 * this is pretty much a hack.
		 */

		if (total_len > wrap) {
			/* <openbsd>
			 * m_devget (rxbufpos - RL_ETHER_ALIGN, wrap + RL_ETHER_ALIGN, 0, ifp, NULL)
			 *           src,                       len
			 * kmemcpy (pkt->frame, (char*)(rxbufpos - RL_ETHER_ALIGN), wrap + RL_ETHER_ALIGN);
			 * pkt->len = wrap;
			 * </openbsd>
			 */

			/* <EVIL> */
			kmemcpy (pkt->frame, (char*)(rxbufpos), wrap);
			pkt->len = wrap - 4;
			/* </EVIL> */

			cur_rx = (total_len - wrap + ETHER_CRC_LEN);
		} else {
			/* <openbsd>
			 * m_devget (rxbufpos - RL_ETHER_ALIGN, total_len + RL_ETHER_ALIGN, 0, ifp, NULL)
			 *           src,                       len
			 * kmemcpy (pkt->frame, (char*)(rxbufpos - RL_ETHER_ALIGN), total_len + RL_ETHER_ALIGN);
			 * pkt->len = total_len + RL_ETHER_ALIGN;
			 * </openbsd>
			 */
			/* <EVIL> */
			kmemcpy (pkt->frame, (char*)(rxbufpos), total_len);
			pkt->len = total_len - 4;
			/* </EVIL> */

			cur_rx += total_len + 4 + ETHER_CRC_LEN;
		}

		/* round up to 32-bit boundary */
		cur_rx = (cur_rx + 3) & ~3;
		CSR_WRITE_2 (dev, RL_CURRXADDR, cur_rx - 16);

		/* handle the packet! */
		network_queue_packet (pkt);
	}
}

/* A frame was downloaded to the chip. It's safe for us to clean up the
 * list buffers */
void
rl_txeof (struct DEVICE* dev) {
	struct RL_DATA* rld = (struct RL_DATA*)dev->data;
	uint32_t txstat;

	/* go through our TX list */
	do {
		txstat = CSR_READ_4 (dev, RL_LAST_TXSTAT (rld));
		if (!(txstat & (RL_TXSTAT_TX_OK | RL_TXSTAT_TX_UNDERRUN | RL_TXSTAT_TXABRT)))
				break;

		if (!(txstat & RL_TXSTAT_TX_OK)) {
			int oldthresh;

			if ((txstat & RL_TXSTAT_TXABRT) ||
					(txstat & RL_TXSTAT_OUTOFWIN)) {
				CSR_WRITE_4 (dev, RL_TXCFG, RL_TXCFG_CONFIG);
				oldthresh = rld->tx_thresh;

				/* error recovery */
				rl_reset (dev);
				rl_init (dev);

				/* if there was a transmit underrun, bump the TX threshold */
				if (txstat & RL_TXSTAT_TX_UNDERRUN)
					rld->tx_thresh = oldthresh + 32;
				return;
			}
		}
		RL_INC (rld->last_tx);
	} while (rld->last_tx != rld->cur_tx);
}

void
rl_irq (struct DEVICE* dev) {
	uint16_t status;

	/* disable interrupts */
	CSR_WRITE_2 (dev, RL_IMR, 0);

	for (;;) {
		status = CSR_READ_2 (dev, RL_ISR);
		if (status)
			CSR_WRITE_2 (dev, RL_ISR, status);

		if ((status & RL_INTRS) == 0)
			break;

		if (status & RL_ISR_RX_OK)
			rl_rxeof (dev);
		if (status & RL_ISR_RX_ERR)
			rl_rxeof (dev);
		if ((status & RL_ISR_TX_OK) || (status & RL_ISR_TX_ERR))
			rl_txeof (dev);
		if (status & RL_ISR_SYSTEM_ERR) {
			rl_reset (dev);
			rl_init (dev);
		}
	}
	
	/* re-enable interrupts */
	CSR_WRITE_2 (dev, RL_IMR, RL_INTRS);

	/* send new packets */
	rl_start (dev);
}

/*
 * Main transmit routine.
 */
void
rl_start (struct DEVICE* dev) {
	struct NETPACKET* pkt;
	struct RL_DATA* rld = (struct RL_DATA*)dev->data;
	size_t len;

	/*
	 * Fetch the next packet to send
	 *
	 * the IRQ will acknowledge when we are done so we can send more
	 *
	 */
/*startagain:*/
	pkt = network_get_next_txbuf (dev);
	if (pkt == NULL)
		/* none, so bail out */
		return;

	/* ensure we don't send packets too tiny */
	len = (pkt->len + pkt->header_len);
	if (len < 60)
		len = 60;

	/* transmit this frame */
	CSR_WRITE_4 (dev, RL_CUR_TXADDR (rld), (uint32_t)(pkt->frame));
	CSR_WRITE_4 (dev, RL_CUR_TXSTAT (rld),
			(RL_TXTHRESH (rld->tx_thresh) |
			len));

	/* update counters */
	dev->tx_frames++; dev->tx_bytes += pkt->len;

	/* free the buffer */
	network_free_packet (pkt);

	/* next */
	RL_INC (rld->cur_tx);

#if 0
	/* do it again, do it again! */
	goto startagain;
#endif

#if 0
	struct DEVICE_CONFIG* cf = (struct DEVICE_CONFIG*)dev->config;
	struct RL_DATA* rld = (struct RL_DATA*)dev->data;
	uint8_t cbuf;

	/* TODO */
	while (dev->txbuf_cur != dev->txbuf_new) {
		/* fetch the counter */
		cbuf = dev->txbuf_cur;

		/* transmit the frame */
		CSR_WRITE_4 (dev, RL_CUR_TXADDR (rld),
				(uint32_t)(dev->txbuf_data + (NETWORK_MTU * cbuf)));
		CSR_WRITE_4 (dev, RL_CUR_TXSTAT (rld),
				RL_TXTHRESH (rld->tx_thresh) |
				dev->txbuf_len[cbuf]);

		/* update counters */
		dev->tx_frames++; dev->tx_bytes += dev->txbuf_len[cbuf];

		RL_INC (rld->cur_tx);
		dev->txbuf_cur = (cbuf + 1) % NETWORK_TXBUFFER_SIZE;
	}
#endif
}

/*
 * Stop the adapter.
 */
void
rl_stop (struct DEVICE* dev) {
	struct RL_DATA* rld = (struct RL_DATA*)dev->data;
	register int i;

	CSR_WRITE_1 (dev, RL_COMMAND, 0x00);
	CSR_WRITE_2 (dev, RL_IMR, 0x00);

	/* free the TX list buffers */
	for (i = 0; i < RL_TX_LIST_CNT; i++) {
		if (rld->tx_chain[i] != NULL) {
			rld->tx_chain[i] = NULL;
			CSR_WRITE_4 (dev, RL_TXADDR0 + i, 0);
		}
	}
}

void
rl_setmulti (struct DEVICE* dev) {
	uint32_t rxfilt;

	rxfilt = CSR_READ_4 (dev, RL_RXCFG);

	CSR_WRITE_4 (dev, RL_MAR0, 0);
	CSR_WRITE_4 (dev, RL_MAR4, 0);

	rxfilt &= ~RL_RXCFG_RX_MULTI;

	CSR_WRITE_4 (dev, RL_RXCFG, rxfilt);
	CSR_WRITE_4 (dev, RL_MAR0, 0);
	CSR_WRITE_4 (dev, RL_MAR4, 0);
}

/*
 * This will initialize the RT8139 card. It will return 0 on failure and
 * 1 on success.
 */
void
rl_init(struct DEVICE* dev) {
	uint32_t i, rxcfg;
	struct RL_DATA* rld = (struct RL_DATA*)dev->data;

	/* stop pending I/O and free all RX/TX buffers */
	rl_stop (dev);
	
	/* initialize our MAC address */
	for (i = 0; i < 6; i++)
		CSR_WRITE_1 (dev, RL_IDR0 + i, dev->ether.hw_addr[i]);

	/* init the RX buffer pointer register */
	CSR_WRITE_4 (dev, RL_RXADDR, /*vtophys (*/rld->rx_buf);

	/* init TX descriptors */
	rl_list_tx_init (dev);

	/* enable transmit and receive */
	CSR_WRITE_1 (dev, RL_COMMAND, RL_CMD_TX_ENB | RL_CMD_RX_ENB);

	/* set the initial TX and RX configuration */
	CSR_WRITE_4 (dev, RL_TXCFG, RL_TXCFG_CONFIG);
	CSR_WRITE_4 (dev, RL_RXCFG, RL_RXCFG_CONFIG);

	/* set the individual bit to receive frames for this host only */
	rxcfg  = CSR_READ_4 (dev, RL_RXCFG);
	rxcfg |= RL_RXCFG_RX_INDIV;

	/* we don't want all frames */
	rxcfg &= ~RL_RXCFG_RX_ALLPHYS;
	CSR_WRITE_4 (dev, RL_RXCFG, rxcfg);

	/* set capture broadcast bit to capture broadcast frames */
	rxcfg |= RL_RXCFG_RX_BROAD;
	CSR_WRITE_4 (dev, RL_RXCFG, rxcfg);

	/* set multicast up */
	rl_setmulti (dev);

	/* enable interrupts */
	CSR_WRITE_2 (dev, RL_IMR, RL_INTRS);

	/* set initial TX treshold */
	rld->tx_thresh = RL_TX_THRESH_INIT;

	/* start RX/TX process */
	CSR_WRITE_4 (dev, RL_MISSEDPKT, 0);

	/* enable receiver and transmitter */
	CSR_WRITE_1 (dev, RL_COMMAND, RL_CMD_TX_ENB | RL_CMD_RX_ENB);

	// MII ??
	
	CSR_WRITE_1 (dev, RL_CFG1, RL_CFG1_DRVLOAD | RL_CFG1_FULLDUPLEX);
}

/*
 * rtl8139_init (struct DEVICE_CONFIG* cf)
 *
 * This will initialize the RealTek card.
 *
 */
int
rtl8139_init (char* name, struct DEVICE_RESOURCES* res) {
	struct DEVICE rdev;
	struct DEVICE* dev;
	struct RL_DATA* rld = (struct RL_DATA*)kmalloc (NULL, sizeof (struct RL_DATA), 0);
	uint16_t rl_id;
	uint16_t rl_did;
	int addr_len, i;

	/* set up a device entry for the card */
	kmemset (&rdev, 0, sizeof (struct DEVICE));
	rdev.name = name;
	rdev.addr_len = ETHER_ADDR_LEN;
	rdev.data = rld;
	rdev.xmit = rl_start;
	kmemcpy (&rdev.resources, res, sizeof (struct DEVICE_RESOURCES));
	dev = device_register (&rdev);

	/* register the IRQ */
	if (!irq_register (res->irq, &rl_irq, dev)) {
		kprintf ("%s: cannot register IRQ %x\n", name, res->irq);
	}

	/* reset the card */
	rl_reset (dev);

	rl_read_eeprom (dev, (void*)&rl_id, RL_EE_ID, RL_EEADDR_LEN1, 1, 0);
	if (rl_id == 0x8129)
		addr_len = RL_EEADDR_LEN1;
	else
		addr_len = RL_EEADDR_LEN0;

	/* fetch station address */
	rl_read_eeprom (dev, (void*)&dev->ether.hw_addr, RL_EE_EADDR, addr_len, 3, 0);

	kprintf ("%s: ethernet address is ", dev->name);
	for (i = 0; i < 6; i++) {
		if (i) kprintf (":");
		kprintf ("%x", dev->ether.hw_addr[i]);
	}
	kprintf ("\n");

	rl_read_eeprom (dev, (void*)&rl_did, RL_EE_PCI_DID, addr_len, 1, 0);

	if (rl_did == 0x8139) {
		rld->type = RL_8139;
	} else {
		kprintf ("%s: unknown type, defaulting to 8139\n", name);
		rld->type = RL_8139;
	}

	/* set the buffers up */
	rld->rx_buf = (addr_t)kmalloc (NULL, RL_RXBUFLEN + 32, 0);
	kmemset ((void*)rld->rx_buf, 0, RL_RXBUFLEN + 32);
	rld->rx_buf += /*rld->rx_buf + */sizeof (uint64_t);

	/* initialize the card */
	rl_init (dev);

	return 1;
}

/* vim:set ts=2 sw=2: */
