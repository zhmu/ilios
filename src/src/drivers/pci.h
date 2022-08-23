/*
 * pci.h - ILIOS PCI driver stuff
 * (c) 2002 Rink Springer, BSD licensed
 *
 */
#include <sys/types.h>

#ifndef __PCI_H__
#define __PCI_H__

struct PCI_SERVICEDIR {
	uint8_t  ident[4];            /* identifier */
	uint32_t entry;               /* entry point */
	uint8_t  revlevel;            /* revision level */
	uint8_t  len;                 /* length */
	uint8_t  cksum;               /* cksum */
	uint8_t  res[5];              /* reserved */
} __attribute__((packed));

/* PCI service signature: "$PCI" */
#define PCI_SERVICE             (('$' << 0) + ('P' << 8) + ('C' << 16) + ('I' << 24))

#define PCIBIOS_PCI_BIOS_PRESENT 0xb101
#define PCIBIOS_PCI_READ_DWORD	 0xb10a


#endif /* __PCI_H__ */

/* vim:set ts=2 sw=2: */
