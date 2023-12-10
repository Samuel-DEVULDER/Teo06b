#
# Makefile pour TEO sur AMIGA
#
# Créé par Samuel Devulder, 08/98.
#

# misc commands
RM   = rm
SED  = sed
ECHO = echo
CP   = cp
PERL = perl

#############################################################################
# compile
#############################################################################
CC=gcc112
UFLAGS=-fcall-used-d2 -fcall-used-d3 -fcall-used-d4
# -fcall-used-d4 
# -fcall-used-a2 
# -fcall-used-a6
# -fcall-used-d5 -fcall-used-a3
OFLAGS=	$(UFLAGS) \
	-fomit-frame-pointer -m68030 -O2 -fstrength-reduce\
	-fno-volatile -fno-volatile-global\
	-fmove-all-movables -freduce-all-givs -fregmove -fgcse\
	-fno-function-cse -fstrength-reduce -frerun-cse-after-loop\
	-frerun-loop-opt
# -fbranch-probabilities -Q

#-msmall-code -fno-function-cse\

#	-fomit-frame-pointer -m68030 -O3\
#	-fno-function-cse
# -msmall-code -fno-volatile -fno-volatile-global -fstrength-reduce\
#	-fmove-all-movables -freduce-all-givs -fregmove 

#	-fomit-frame-pointer -m68030 -O3 -fno-volatile -fno-volatile-global

#	-fmove-all-movables -freduce-all-givs -fregmove 
# 	-fcall-used-a2 -fcall-used-a3\

FPIC=-msmall-code
#-fbaserel -msmall-code -DSMALL_MODEL

CFLAGS=$(OFLAGS) $(FPIC) -Iamiga/ -I. -Wall -DFAST

#############################################################################
# link
#############################################################################
LN=$(CC)
LFLAGS=-mstackextend -noixemul $(FPIC)

OBJS=	debug.o disk.o index.o main.o mapper.o mc6809_1.o\
        mc6809_2.o mem_mng.o setup.o setup2.o keyb.o zfile.o\
	amiga/gui.o amiga/graph.o amiga/sound.o amiga/events.o

teo: $(OBJS)
	$(LN) $(OBJS) -o $@ $(LFLAGS)

teo.full: $(OBJS)
	$(LN) $(OBJS) -o $@ $(LFLAGS)

gcc270%:
	make "CC=gcc" "OFLAGS=$(UFLAGS) -fomit-frame-pointer -m68030 -O3" $*

full:
	make "CFLAGS=$(OFLAGS) $(FPIC) -Iamiga/ -I. -Wall" teo.full

debug:
	make OFLAGS=-g "LFLAGS=-g -lamiga"
	
profile:
	make "OFLAGS=-p -D__PROFILE__ -O3 -m68030" "LFLAGS=-p -lamiga"

backup:
	-rm teo06sam.lha
	lha -r a teo06sam.lha "#?.[ch]" Makefile ChangeLog Readme*

useallregs:
	make "UFLAGS=-fcall-used-d2 -fcall-used-d3 -fcall-used-d4\
		     -fcall-used-d5 -fcall-used-d6 -fcall-used-d7\
		     -fcall-used-a2 -fcall-used-a3 -fcall-used-a4\
		     -fcall-used-a6"

usenoregs:
	make "UFLAGS="

usefewregs:
	make "UFLAGS=-fcall-used-d2 -fcall-used-d3 -fcall-used-a6"

ixemul:
	-rm teo
	make "LFLAGS="

clean:
	-$(RM) $(OBJS) $(OBJS:.o=.s)

# modify Makefile to reflect dependencies
depend: 
	$(SED)	 > Makefile.d \
		-e '/^# dependencies/,/^# end of dependencies/ d' Makefile 
	$(ECHO) >> Makefile.d "# dependencies"
	$(CC) $(CFLAGS) >> Makefile.d -MM $(OBJS:.o=.c)
	$(ECHO) >> Makefile.d "# end of dependencies"
	$(CP) Makefile Makefile.bak
	$(CP) Makefile.d Makefile

# general rule
.c.o:
	$(CC) -S $*.c -o $*.asm $(CFLAGS)
	$(PERL) perl_opt.pl $*.asm >$*.s
	$(CC) -c $*.s -o $*.o $(CFLAGS)
	rm $*.asm

# dependencies
debug.o: debug.c flags.h
disk.o: disk.c flags.h regs.h mc6809.h mapper.h graph.h diskio.h \
 zfile.h
index.o: index.c flags.h mapper.h index.h regs.h
main.o: main.c flags.h regs.h mouse.h mapper.h debug.h mc6809.h \
 setup.h setup2.h index.h graph.h keyb.h keyb_int.h sound.h diskio.h \
 main.h zfile.h
mapper.o: mapper.c flags.h keyb.h sound.h mapper.h graph.h regs.h
mc6809_1.o: mc6809_1.c flags.h mapper.h graph.h index.h regs.h
mc6809_2.o: mc6809_2.c flags.h mapper.h index.h regs.h mc6809.h \
 graph.h
mem_mng.o: mem_mng.c
setup.o: setup.c flags.h mapper.h regs.h mc6809.h index.h
setup2.o: setup2.c flags.h
keyb.o: keyb.c flags.h mapper.h keyb_int.h keyb.h
zfile.o: zfile.c flags.h zfile.h
gui.o: amiga/gui.c flags.h main.h mapper.h sound.h regs.h graph.h \
 diskio.h keyb.h zfile.h
graph.o: amiga/graph.c flags.h mapper.h graph.h main.h keyb.h \
 keyb_int.h
sound.o: amiga/sound.c flags.h mapper.h sound.h
events.o: amiga/events.c flags.h diskio.h mapper.h graph.h main.h \
 mouse.h keyb.h keyb_int.h
# end of dependencies
