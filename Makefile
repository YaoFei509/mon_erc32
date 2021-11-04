###########################################################################
#
# Filename:
#
#   Makefile
#
# Description:
#
#   Makefile to build and run the example programs
#
#   There are three targets in this Makefile:
#
#     all    - compile all
#     run    - execute erc_mon on the simulator
#     clean  - delete derived files
#
###########################################################################

SHELL = /bin/sh

VER=
XGC=/opt/erc32-ada$(VER)/bin/
prefix = $(XGC)erc-elf

CC       = $(prefix)-gcc
LD       = $(prefix)-ld
GNATMAKE = $(prefix)-gnatmake -Isrc
GNATCHOP = $(prefix)-gnatchop
OBJCOPY  = $(prefix)-objcopy
OBJDUMP  = $(prefix)-objdump
RUN      = $(prefix)-run

#Use network port as UART 
RUN_FLAG = -bcypqrsES -a "-V -uart1 6950 -wdog -rambs 8 -freq 16"

CFLAGS = -g -Wall -D__USE_UARTA

#DEBUG啰嗦模式，显示一大堆调试信息
CFLAGS += -D__DEBUG

#可以写PROM区域的AT28HC256，万分小心
CFLAGS += -D__UPDATE_PROM

#################################################################################
ERC32_MON_LDFLAGS = -T ldscripts/mon_erc32_ram.x      -Wl,-Map=mon_erc32.map
PROM_LDFLAGS      = -T ldscripts/mon_erc32_prom.x     -Wl,-Map=mon_erc32_prom.map
PROM200_LDFLAGS   = -T ldscripts/mon_erc32_prom_200.x -Wl,-Map=mon_erc32_prom_200.map

OBJS = mon_erc32.o erc32_io.o eeprom.o

TARGETS   = mon_erc32

all: $(TARGETS)

mon_erc32: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(ERC32_MON_LDFLAGS) $(OBJS)
	$(OBJCOPY) -O ihex $@ $@.hex

run: $(TARGETS) 
	@echo run "nc localhost 6950 < hanoi.hex" at another terminal window.
	$(RUN) $(RUN_FLAG) $<

prom: $(OBJS) 
	$(CC) $(CFLAGS) $(PROM_LDFLAGS)    -o $(TARGETS)_prom.hex $(OBJS)
	$(CC) $(CFLAGS) $(PROM200_LDFLAGS) -o $(TARGETS)_prom_200.hex $(OBJS)

downld: prom
	cp $(TARGETS)_prom_200.hex /dev/ttyMXUSB1

asm: $(TARGETS)
	$(OBJDUMP) -Sa $< > $<.asm

log:
	git pull
	git log --format=short --graph > ChangeLog

# target to delete generated files

clean:
	-rm -rf $(TARGETS) $(TESTSUITS) *.bin *.hex *.o *.s a.out *.map *~ *.asm 

###########################################################################
#  End of Makefile
###########################################################################
