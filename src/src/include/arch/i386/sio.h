/*
 * serial.h - XeOS Serial IO Module Include File
 * (c) 2002 Rink Springer, BSD licensed
 *
 * This module handles all serial IO stuff.
 *
 */

#define SIO_RBP		0x00			/* (R) receive port */
#define SIO_THR		0x00			/* (W) transmit port */
#define SIO_IER		0x01			/* (R/W) interrupt enable */
#define SIO_LSB		0x00			/* (R/W) divisor latch, lo */
#define SIO_MSB		0x01			/* (R/W) divisor latch, hi */
#define SIO_IIR		0x02			/* (R) int ident register */
#define SIO_FCR		0x02			/* (W) FIFO control reg */
#define SIO_LCR		0x03			/* line control register */
#define SIO_MCR		0x04			/* modem control register */
#define SIO_LSR		0x05			/* line status register */
#define SIO_MSR		0x06			/* modem status register */
#define SIO_SCR		0x07			/* scratch register */

/* IER */
#define SIO_IER_EDSSI	0x08			/* delta status signals */
#define SIO_IER_ELSI	0x04			/* line status interrupt */
#define SIO_IER_ETBEI	0x02			/* transmitter buffer empty */
#define SIO_IER_ETBFI	0x01			/* transmitter full */

/* IIR */
#define SIO_IIR_FIFOE0	0x80			/* FIFO enable */
#define SIO_IIR_FIFOE1	0x40			/* FIFO enable */
#define SIO_IIR_IID2	0x08			/* Interrupt ID2 */
#define SIO_IIR_IID1	0x04			/* Interrupt ID1 */
#define SIO_IIR_IID0	0x02			/* Interrupt ID0 */
#define SIO_IIR_PENDING	0x01			/* interrupt pending */

/* LCR */
#define SIO_LCR_DLAB	0x80			/* divisior latch access bit */
#define SIO_LCR_SBR	0x40			/* set break */
#define SIO_LCR_STICK	0x20			/* stick parity */
#define SIO_LCR_EVEN	0x10			/* even parity */
#define SIO_LCR_ENABLE	0x08			/* parity enable */
#define SIO_LCR_STOP	0x04			/* stop bit */
#define SIO_LCR_WORD0	0x02			/* word length */
#define SIO_LCR_WORD1	0x01			/* word length */

/** LSR */
#define SIO_LSR_FIFO	0x80			/* error pending */
#define SIO_LSR_TEMT	0x40			/* transmitter empty */
#define SIO_LSR_THRE	0x20			/* transmitter hold reg empty */
#define SIO_LSR_BREAK	0x10			/* broken line detected */
#define SIO_LSR_FE	0x08			/* framing error */
#define SIO_LSR_PE	0x04			/* parity error */
#define SIO_LSR_OE	0x02			/* overrun error */
#define SIO_LSR_RBF	0x01			/* recv buffer full */

/* MSR */
#define SIO_MSR_DCD	0x80			/* Data Carrier Detect */
#define SIO_MSR_RI	0x40			/* Ring Indicator */
#define SIO_MSR_DSR	0x20			/* Data Set Ready */
#define SIO_MSR_CTS	0x10			/* Clear To Send */
#define SIO_MSR_DDCD	0x08			/* Delta Data Carrier Detect */
#define SIO_MSR_TERI	0x04			/* Trailing Edge Ring Ind. */
#define SIO_MSR_DDSR	0x02			/* Delta Data Set Ready */
#define SIO_MSR_DCTS	0x01			/* Delta Clear To Send */

/* MCR */
#define SIO_MCR_OUT2	0x08			/* interrupt bit */

/* types */
#define SIO_TYPE_8250	0			/* 8250 */
#define SIO_TYPE_16450	1			/* 16450 */
#define SIO_TYPE_16550	2			/* 16550 */
#define SIO_TYPE_16550A	3			/* 16550A */

void arch_sio_init();
void arch_sio_send (uint8_t ch);
