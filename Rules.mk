include                $(MK_SHARE)Core/Main.dirstack.mk
MK_$d               := $/Rules.mk
MK                  += $(MK_$d)
#
#      ------------ --

ARDUINODIR := $(shell realpath ./arduinodir/)

arduinodir-libraries-relink::
	for f in libraries/*; do \
		F=$$(basename $$f); \
		T=$$(realpath arduinodir/libraries)/$$F; \
		[ -e "$$T" ] && [ -L "$$T" ] && { \
			rm -v $$T; \
		} || { \
			echo -n; \
		};\
		[ ! -e "$$T" ] && { \
			ln -sv $$(pwd -P)/$$f $$T; \
		} || { \
			echo -n; \
		};\
	done

# add devices here, wildcard checks with FS for available devices
PORTS := $(wildcard /dev/tty.usbserial-* /dev/ttyUSB*)
# get list of actually connected devices, select one
port ?= 1
PORT := $(word $(port),$(PORTS))
$(info $(shell $(call log,header2,PORTS[$(port)],$(PORT))))


ports:
	@\
		echo "Available ports:";\
		for x in $(PORTS); do echo $$x; done

listen: D := $(PORT)
listen: B := 57600
#listen: B := 38400
listen:
	@\
	$(ll) attention $@ "Starting minicom @$(B) baud.." $(D);\
	minicom -D $(D) -b $(B) minirc.arduino
	@\
	$(ll) ok $@ "minicom ended." $(D)

screen: D := $(PORT)
screen: B := 57600
screen:
	@\
	$(ll) attention $@ "Calling screen $(D) $(B)..";\
	screen $(D) $(B)

#
#DIR                 := $/mydir
#include                $(call rules,$(DIR)/)
#
# DMK += $/dynamic-makefile.mk
# DEP += $/generated-dependency
# TRGT += $/build-target
# TEST += $/testtarget
# XXX: cleanable only noticed on 'make clean'
#ifneq ($(call contains,$(MAKECMDGOALS),clean),)
# XXX: using find here is so wastefull (otherwise), there is no cache at all?
CLN += $(shell find $/ -name .dep -or -name .lib -o -name *.o -o -name *.swp -o -name *.swo)
#endif

OLD ?= 1

ifeq ($(OLD),1)
include Rules.old.mk
else
include Rules.new.mk
endif

include Rules.simple-m8.mk

#      ------------ -- 
#
include                $(MK_SHARE)Core/Main.dirstack-pop.mk
# vim:noet:

