/*
 * pci.c - ILIOS PCI handling code
 * (c) 2003 Rink Springer, BSD licensed
 *
 * This module handles all PCI device stuff.
 *
 * The PCI specification from
 * http://www.nondot.org/sabre/os/incoming/pcibios.pdf was used for this code,
 * (c) The PCI Special Interest Group
 *
 * Some of this code is based on Linux's arch/i386/pci/pcbios.c. The actual
 * bus scanning code is based on
 * http://www.experts-exchange.com/Programming/Programming_Languages/C/
 * Q_10060434.html by nils pipenbrinck.
 *
 */
#include <sys/device.h>
#include <sys/irq.h>
#include <sys/network.h>
#include <sys/kmalloc.h>
#include <sys/types.h>
#include <lib/lib.h>
#include <md/config.h>
#include <md/pio.h>
#include "pci.h"

#define xPCI_VERBOSE

struct {
	uint32_t	address;
	uint16_t	segment;
} pci_indirect = { 0, KCODE32_SEL };

uint8_t pci_last_bus, rl_count = 0;

int rtl8139_init (char* name, struct DEVICE_RESOURCES* res);

/*
 * This will scan for the PCI service directory. It will return NULL on failure
 * or the address on success.
 */
struct PCI_SERVICEDIR*
pci_scan_servicedir() {
	struct PCI_SERVICEDIR* sd = (struct PCI_SERVICEDIR*)0xE0000;

	/* try to locate the PCI bios */
	while ((uint32_t)sd <= 0xFFFF0) {
		/* got it? */
		if ((sd->ident[0] == '_') && (sd->ident[1] == '3') &&
				(sd->ident[2] == '2') && (sd->ident[3] == '_')) {
			/* yes! TODO: VERIFY CHECKSUM */
			return sd;
		}

		/* next */
		sd++;
	}

	/* too bad */
	return NULL;
}

/*
 * This will read a doubleworld from the PCI bus.
 */
uint32_t
pci_read_dword (uint8_t bus, uint8_t dev, uint32_t reg) {
	uint32_t bx = ((bus << 8) | (dev << 3));
	uint32_t value;

	asm ("lcall *(%%esi)\ncld\n"
			 : "=c" (value)
			 : "a" (PCIBIOS_PCI_READ_DWORD),
			   "b" (bx),
				 "D" (reg),
			   "S" (&pci_indirect));

	return value;
}

/*
 * This will attach a PCI card as an NE2K card.
 */
int
rl_pci_attach (uint32_t io, uint8_t irq) {
	struct DEVICE_RESOURCES rl_res;
	char* name;

	/* set up the resources */
	rl_res.port = io;
	rl_res.irq = irq;
	rl_res.drq = 0;

	name = (char*)kmalloc (NULL, 4, 0);
	kstrcpy (name, "rl?");
	name[2] = 0x30 + rl_count;

	rl_count++;
	return rtl8139_init (name, &rl_res);
}

/*
 * This will initialize the PCI bus.
 *
 */
void
pci_init() {
	struct PCI_SERVICEDIR* sd = pci_scan_servicedir();
	uint32_t return_code, address, length, entry;
	uint32_t signature, eax, ebx, ecx;
	struct {
		uint32_t	address;
		uint16_t	segment;
	} bios32_indirect = { 0, KCODE32_SEL };
	uint8_t status;
	uint8_t busno, devno;
	uint8_t major_rev, minor_rev, hw_mech;
	uint32_t vendor, classcode, base_mem, irq;
	uint32_t pio_addr;

	if (sd == NULL) {
		kprintf ("BIOS32 services not detected\n");
		return;
	}

	kprintf ("BIOS32 services detected at 0x%x\n", sd);
	bios32_indirect.address = sd->entry;

	/* ask the bios32 services for the real PCI bios
	 * taken from Linux */
	asm ("lcall *(%%edi)\ncld"
			 : "=a" (return_code),
			   "=b" (address),
			   "=c" (length),
			   "=d" (entry)
			 : "0" (PCI_SERVICE),
			   "1" (0),
				 "D" (&bios32_indirect));

	/* success? */	
	if (return_code & 0xff) {
		/* no. bail out */
		kprintf ("PCI BIOS not present [return code is %x]\n", (return_code & 0xff));
		return;
	}

	/* call the PCI bios */
	pci_indirect.address = (address + entry);
	asm ("lcall *(%%edi)\ncld\n"
			 "jc 1f\n"
			 "xor %%ah,%%ah\n"
			 "1:"
			 : "=d" (signature),
			   "=a" (eax),
			   "=b" (ebx),
			   "=c" (ecx)
			 : "1" (PCIBIOS_PCI_BIOS_PRESENT),
			   "D" (&pci_indirect)
			 : "memory");

	status       = (eax >> 8) & 0xff;
	hw_mech      = (eax       & 0xff);
	major_rev    = (ebx >> 8) & 0xff;
	minor_rev    = (ebx       & 0xff);
	pci_last_bus = (ecx       & 0xff);

	/* yay! */
	kprintf ("PCI BIOS %u.%u detected, %u busses\n", major_rev, minor_rev,pci_last_bus);

	/* scan the busses */
	for (busno = 0; busno <= pci_last_bus; busno++) {
		/* scan all devices */
		for (devno = 0; devno < 32; devno++) {
			/* fetch the vendor number */
			vendor = pci_read_dword (busno, devno, 0);

			/* got a device here? */
			if (vendor != -1) {
				/* yes. fetch the class code and resources */
				classcode = pci_read_dword (busno, devno, 8) >> 8;
				base_mem = pci_read_dword (busno, devno, 0x10);
				irq = pci_read_dword (busno, devno, 0x3c) & 0xff;
				if (base_mem & 1)
					pio_addr = (base_mem & 0xfffe);
				else
					pio_addr = 0;

#ifdef PCI_VERBOSE
				kprintf ("PCI %x:%x: vendor %x device %x class %x\n", busno, devno, vendor & 0xffff, vendor >> 16, classcode);
#endif /* PCI_VERBOSE */

#if 0
				/* hack: NE2000 winbond card */
				if (((vendor & 0xffff) == 0x1050) && ((vendor >> 16) == 0x940)) {
					/* attach it */
					ne2k_pci_attach (pio_addr, irq);
				}
#endif
				/* realtek? */
				if ((((vendor >> 16) & 0xffff) == 0x8139)) {
					/* yes. attach it */
					rl_pci_attach (pio_addr, irq);
				}
			}
		}
	}
}

/* vim:set ts=2 sw=2: */
