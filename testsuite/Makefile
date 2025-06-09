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
GNATMAKE = $(prefix)-gnatmake -Isrc -i
GNATCHOP = $(prefix)-gnatchop
OBJCOPY  = $(prefix)-objcopy
OBJDUMP  = $(prefix)-objdump
RUN      = $(prefix)-run

CFLAGS = -g -Wall

#DEBUG啰嗦模式，显示一大堆调试信息
CFLAGS += -D__DEBUG

#ldscript for user applications
LDFLAGS  = -T erc32.M -Wl,-Map=$@.map 

TESTSUITS = hanoi pascal man

all:  $(TESTSUITS)

hanoi: hanoi.adb
	$(GNATMAKE) $@ -largs  $(LDFLAGS)
	$(OBJCOPY) -O ihex $@ $@.hex

pascal: pascal.adb 
	$(GNATMAKE) $@ -largs  $(LDFLAGS) 
	$(OBJCOPY) -O ihex $@ $@.hex


man: man.adb
	$(GNATMAKE) $@ -largs  $(LDFLAGS) 
	$(OBJCOPY) -O ihex $@ $@.hex 

run: hanoi
	$(RUN) $<
log:
	git pull
	git log --format=short --graph > ChangeLog

# target to delete generated files

clean:
	-rm -rf  $(TESTSUITS) *.bin b~* *.hex *.o *.s a.out *.map *.ali *~ *.asm src/*

# chop
%.ad[sb]: %.ada
	test -d src || mkdir src
	$(GNATCHOP) -r -w $< src	

###########################################################################
#  End of Makefile
###########################################################################
