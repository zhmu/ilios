/*
 * init.c - XeOS i386 initialization code
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 * This code is heavily based on Yoctix, most residing in src/arch/i386/, (c)
 * 1999 Anders Gavare.
 *
 * This code will handle setting up the i386-dependant stuff.
 *
 */
#include <md/config.h>
#include <md/gdt.h>
#include <md/init.h>
#include <md/interrupts.h>
#include <lib/lib.h>
#include <sys/kmalloc.h>
#include <sys/irq.h>
#include <config.h>

uint32_t ticks_till_switch;
signed int switch_ratio;

/* declared in startup.c */
extern uint32_t kernelpagedir;

/* forward declarations for the assembly code */
void timer_asm();
void exc0_asm();
void exc1_asm();
void exc2_asm();
void exc3_asm();
void exc4_asm();
void exc5_asm();
void exc6_asm();
void exc7_asm();
void exc8_asm();
void exc9_asm();
void exca_asm();
void excb_asm();
void excc_asm();
void excd_asm();
void exce_asm();
void excf_asm();
void irq0_asm();
void irq1_asm();
void irq2_asm();
void irq3_asm();
void irq4_asm();
void irq5_asm();
void irq6_asm();
void irq7_asm();
void irq8_asm();
void irq9_asm();
void irqa_asm();
void irqb_asm();
void irqc_asm();
void irqd_asm();
void irqe_asm();
void irqf_asm();

/* own declarations */
uint8_t* gdt;
uint8_t* idt;

/*
 * This will set up GDT entry [no] according to the information supplied.
 */
void
gdt_set_entry (uint8_t no, uint32_t limit, uint32_t addr, uint8_t segtype, uint8_t desctype, uint8_t dpl, uint8_t pbit, uint8_t avl, uint8_t opsize, uint8_t dbit, uint8_t gran) {
	uint8_t* p = (uint8_t*)(gdt + (no * 8));

	/* fill the entry out */
	p[0]  = limit & 0xff;			/* segment limit */
	p[1]  = (limit >> 8) & 0xff;
	p[2]  = addr & 255;				/* base address */
	p[3]  = (addr >> 8) & 255;
	p[4]  = (addr >> 16) & 255;
	p[5]  = segtype;
	p[5] |= (desctype * 16);
	p[5] |= (dpl * 32);
	p[5] |= (pbit * 128);
	p[6]  = ((limit >> 16) & 15);
	p[6] |= (avl * 16);
	p[6] |= (opsize * 32);
	p[6] |= (dbit * 64);
	p[6] |= (gran *  128);
	p[7]  = (addr >> 24) & 255;
}

/* 
 * This will set up the specified interrupt up.
 */
void
interrupts_set_entry (uint8_t no, void* handler, uint16_t sel, uint8_t type, uint8_t dpl) {
	uint8_t* p = (uint8_t*)(idt + (no * 8));

	p[0] = (uint32_t)handler & 0xff;
	p[1] = ((uint32_t)handler >> 8) & 0xff;
	p[2] = sel & 0xff;
	p[3] = (sel >> 8) & 0xff;
	p[4] = 0;
	p[5] = 128 + (dpl * 32) + type;
	p[6] = ((uint32_t)handler >> 16) & 0xff;
	p[7] = ((uint32_t)handler >> 24) & 0xff;
}

/*
 * This will create and activate our new GDT.
 */
void
gdt_init() {
	int sz;
	uint8_t gdt_address[6];

	/* get the size */
	sz = 32;

	/* allocate memory */
	gdt = (unsigned char*)kmalloc (NULL, sz, 0);

	/* first of all, clean out the entire GDT, so the NULL descriptor is
	   correct */
	kmemset (gdt, 0, sz);

	/* 1: kernel code */
	gdt_set_entry (1, 0xfffff, 0x0, GDT_SEGTYPE_EXEC | GDT_SEGTYPE_READ_CODE,
	               GDT_DESCTYPE_CODEDATA, 0, 1, 0, 1, 1, 1);

	/* 2: kernel data */
	gdt_set_entry (2, 0xfffff, 0x0, GDT_SEGTYPE_WRITE_DATA,
	               GDT_DESCTYPE_CODEDATA, 0, 1, 0, 1, 1, 1);

	/* 3: 16 bit code */
	gdt_set_entry (3, 0xffff, 0x0, GDT_SEGTYPE_EXEC | GDT_SEGTYPE_READ_CODE,
	               GDT_DESCTYPE_CODEDATA, 0, 1, 0, 0, 0, 0);

	/* use the new GDT */
	gdt_address[0] = (sz - 1) & 0xff;
	gdt_address[1] = (sz - 1) >> 8;
	gdt_address[2] = ((uint32_t)gdt & 0xff);
	gdt_address[3] = ((uint32_t)gdt >>  8) & 0xff;
	gdt_address[4] = ((uint32_t)gdt >> 16) & 0xff;
	gdt_address[5] = ((uint32_t)gdt >> 24) & 0xff;

	/* go! */
	__asm__ ("lgdt (%0)" : : "r" (&gdt_address[0]));

	/* ensure it's all correct now */
	__asm__ ("mov $0x10, %ax");
	__asm__ ("mov %ax, %ds");
	__asm__ ("mov %ax, %es");
	__asm__ ("mov %ax, %ss");
	__asm__ ("mov %ax, %fs");
	__asm__ ("mov %ax, %gs");
	__asm__ ("jmp 1f\n1:\n");
}

/*
 * This will set up the interrupts.
 */
void
interrupts_init() {
	uint8_t  idt_address[6];

	/* set the PIC up first (this sequence is copied from Yoctix) to remap
	 * interrupts to a sensible location */
	asm ("mov $0x20,%dx\nmov $0x11,%al\nout %al,%dx"); /* outb (0x20, 0x11) */
	asm ("mov $0xa0,%dx\nmov $0x11,%al\nout %al,%dx"); /* outb (0xa0, 0x11) */
	asm ("mov $0x21,%dx\nmov $0x20,%al\nout %al,%dx"); /* outb (0x21, 0x20) */
	asm ("mov $0xa1,%dx\nmov $0x28,%al\nout %al,%dx"); /* outb (0xa1, 0x28) */
	asm ("mov $0x21,%dx\nmov $0x04,%al\nout %al,%dx"); /* outb (0x21, 0x04) */
	asm ("mov $0xa1,%dx\nmov $0x02,%al\nout %al,%dx"); /* outb (0xa1, 0x02) */
	asm ("mov $0x21,%dx\nmov $0x01,%al\nout %al,%dx"); /* outb (0x21, 0x01) */
	asm ("mov $0xa1,%dx\nmov $0x01,%al\nout %al,%dx"); /* outb (0xa1, 0x01) */
	asm ("mov $0x21,%dx\nxor %al,%al\nout %al,%dx");   /* outb (0x21, 0x00) */
	asm ("mov $0xa1,%dx\nxor %al,%al\nout %al,%dx");   /* outb (0xa1, 0x00) */
	
	/* allocate memory for the IDT */
	idt = (unsigned char*)kmalloc (NULL, (8 * 256), 0);

	/* clean out the entire IDT */
	kmemset (idt, 0, (8 * 256));

	/* set the exception handlers */
	interrupts_set_entry (0x00, (void*)exc0_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x01, (void*)exc1_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x02, (void*)exc2_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x03, (void*)exc3_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x04, (void*)exc4_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x05, (void*)exc5_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x06, (void*)exc6_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x07, (void*)exc7_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x08, (void*)exc8_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x09, (void*)exc9_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x0a, (void*)exca_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x0b, (void*)excb_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x0c, (void*)excc_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x0d, (void*)excd_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x0e, (void*)exce_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x0f, (void*)excf_asm, KCODE32_SEL, I386_INT_GATE, 0);

	/* set all irq handlers */
	interrupts_set_entry (0x20, (void*)&irq0_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x21, (void*)&irq1_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x22, (void*)&irq2_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x23, (void*)&irq3_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x24, (void*)&irq4_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x25, (void*)&irq5_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x26, (void*)&irq6_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x27, (void*)&irq7_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x28, (void*)&irq8_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x29, (void*)&irq9_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x2a, (void*)&irqa_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x2b, (void*)&irqb_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x2c, (void*)&irqc_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x2d, (void*)&irqd_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x2e, (void*)&irqe_asm, KCODE32_SEL, I386_INT_GATE, 0);
	interrupts_set_entry (0x2f, (void*)&irqf_asm, KCODE32_SEL, I386_INT_GATE, 0);

	/* build the address */
	idt_address[0] = ((8 * 256) - 1) & 0xff;
	idt_address[1] = ((8 * 256) - 1) >> 8;
	idt_address[2] =  (uint32_t)idt & 0xff;
	idt_address[3] = ((uint32_t)idt >>  8) & 0xff;
	idt_address[4] = ((uint32_t)idt >> 16) & 0xff;
	idt_address[5] = ((uint32_t)idt >> 24) & 0xff;

	/* here goes nothing... */
	__asm__ ("lidt (%0)" : : "r" (&idt_address[0]));
}

/*
 * This will handle our timer stuff.
 */
void
timer_init () {
	int v;

	/* program the timer */
#define TIMER_FREQ      1193182
#define TIMER_DIV(x) ((TIMER_FREQ+(x)/2)/(x))

	v = TIMER_DIV(HZ);
	asm ("mov $0x43,%dx\nmov $0x34,%al\nout %al,%dx"); /* outb (0x43, 0x34) */
	asm ("out %0,%1" : : "a" (v & 0xff), "id" (0x40)); /* outb (0x40, v % 0xff) */
	asm ("out %0,%1" : : "a" (v >>   8), "id" (0x40)); /* outb (0x40, v >>   8) */

	/* officially register the IRQ, so no one else can grab it */
	irq_register (0, (void*)&timer_asm, NULL);

	/* the timer is time-critical, so just call it at once, to avoid the IRQ
	   manager bloat */
	interrupts_set_entry (0x20, (void*)&timer_asm, KCODE32_SEL, I386_INT_GATE, 0);
}

#ifdef SUPPORT_GDB
void
exceptionHandler (int num, void* addr) {
	interrupts_set_entry (num, addr, KCODE32_SEL, I386_INT_GATE, 0);
}
#endif /* SUPPORT_GDB */

/*
 * This will initialize the machine dependant stuff.
 */
void
arch_init() {
	/* initialize the GDT */
	gdt_init();

	/* initialize the interrupts and exceptions */
	interrupts_init();

	/* initialize the timer */
	timer_init();
}

/* vim:set ts=2 sw=2: */
