TARGET  = kernel.sys
OBJS	= main/main.o main/version.o \
	cli/cli.o cli/cmd.o \
	arch/i386/stub.o arch/i386/init.o arch/i386/memory.o \
	arch/i386/timer_asm.o arch/i386/halt.o arch/i386/pio.o \
	arch/i386/startup.o arch/i386/int_asm.o arch/i386/exceptions.o \
	arch/i386/interrupts.o arch/i386/console.o arch/i386/timer.o arch/i386/sio.o \
	sys/irq.o sys/kmalloc.o sys/device.o sys/network.o \
	lib/kprintf.o lib/panic.o lib/string.o lib/input.o \
	lib/i386/kmemcmp.o lib/i386/memcpy.o lib/i386/memset.o \
	lib/i386/strcat.o lib/i386/strchr.o lib/i386/strcmp.o lib/i386/strcpy.o lib/i386/strlen.o \
	lib/i386/strncmp.o \
	lib/i386/ntohl.o lib/i386/ntohs.o \
	lib/i386/htonl.o lib/i386/htons.o \
	drivers/pci.o drivers/ne.o drivers/rtl8139.o drivers/lo.o \
	drivers/ep.o \
	netipv4/arp.o netipv4/cksum.o netipv4/icmp.o netipv4/ip.o \
	netipv4/ipv4.o netipv4/route.o netipv4/udp.o netipv4/tcp.o \
	net/dhcp.o net/socket.o net/dns.o net/stats.o
ARCH	= i386
CFLAGS	= -nostdinc -Iinclude
CFLAGS  += -Wall -Werror
CFLAGS	+= -D__KERNEL -DARCH=${ARCH} -DSUPPORT_GDB
#CFLAGS	+= -DHAVE_DISASM

include		../mk/defs.mk

.ORDER:		version.h main/version.o kernel.sys

kernel.sys:	version.h $(OBJS) $(LIBS)
		$(LD) -Ttext 0x100000 -e __start -o kernel.sys $(OBJS) $(LIBS)
		$(CP) kernel.sys kernel.debug.sys
		$(STRIP) --remove-section=.comment --remove-section=.note kernel.sys
		$(RM) -f version.h version.c version.o

kernel.aout:	version.h $(OBJS) $(LIBS)
		$(LD) --oformat a.out-i386-linux -Ttext 0x100000 -e __start -o kernel.sys $(OBJS) $(LIBS)
		$(CP) kernel.aout kernel.debug.aout
		$(RM) -f version.h version.c version.o

clean:
		$(RM) -f kernel.sys kernel.debug.sys version.h version.c $(OBJS)

GENERATION:
		echo -1 > GENERATION

version.h:	Makefile GENERATION
		echo `cat GENERATION`+1|bc > GENERATION.tmp
		mv -f GENERATION.tmp GENERATION
		@echo "char* ILIOS_VERSION();" > version.h
		@echo "#define VERSION \"ILIOS/${ARCH} #`cat GENERATION`: `date`\n  `whoami`@`hostname`:`pwd`\"" >> version.h

floppy-linux:	kernel.sys ../tools/base-grub.flp
		$(CP) ../tools/base-grub.flp /tmp/ilios.flp
		sudo mount /tmp/ilios.flp /mnt -t msdos -o loop,uid=rink,sync
		$(CP) kernel.sys /mnt/boot/ilios
		sudo umount /mnt

floppy-bsd:	kernel.sys ../tools/base-grub.flp
		$(CP) ../tools/base-grub.flp /tmp/ilios.flp
		vnconfig -c /dev/vn0 /tmp/ilios.flp
		mount -t msdos /dev/vn0 /mnt
		$(CP) kernel.sys /mnt/boot/ilios
		umount /mnt
		vnconfig -u vn0
		cp /tmp/ilios.flp /data/tmp

floppy:		floppy-bsd

../tools/base-grub.flp:	../tools/Makefile
			cd ../tools && make base-grub.flp

copy:			kernel.sys
			mcopy -o kernel.sys a:/boot/ilios

arch/i386/realmode.o:		md/realmode.s
			$(CC) -o md/realmode.o md/realmode.s

# vim:set ts=8 sw=8:
