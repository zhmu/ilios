# standard executables
LD		= ld
CP		= cp
STRIP		= strip
RM		= rm
MV		= mv
AS		= as
GZ		= gzip

# c files
.c.o:	      
		$(CC) $(CFLAGS) -c -o $@ $<

# assembly files
.s.o:
		$(AS) -o $@ $<
.S.o:
		$(CC) -c -o $@ $<

