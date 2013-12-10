MK_$d               := $/Rules.new.mk
MK                  += $(MK_$d)
#
#      ------------ --


#DIR                 := $/mydir
#include                $(call rules,$(DIR)/)
#
# DMK += $/dynamic-makefile.mk
# DEP += $/generated-dependency
# TRGT += $/build-target
# CLN += $/tmpfile
# TEST += $/testtarget

CLN += $(shell find $/ -name .dep -or -name .lib -o -name *.o -o -name *.swp -o -name *.swo)

ARDUINO_DIR = /usr/share/arduino

PORTS = \
		USB0=/dev/ttyUSB0 \
		USB1=/dev/ttyUSB1 \
		USB2=/dev/ttyUSB2 \
		usbserial=/dev/tty.usbserial-A900TTH0

BOARDS_FILES := $(wildcard  \
	$(ARDUINO_DIR)/hardware/*/boards.txt \
	./hardware/*/boards.txt)

# TODO filter boards for existing entry

# BRD
# PRT
define build-arduino-firmware
	[ -e "$<" ] && { \
		$(ll) attention "$@" "Building flash image, board $(BRD).." "$(<)"; \
	} || {  \
		echo "Missing source"; \
		$(ll) error "$@" "Missing source" "$<"; \
		exit 1; \
	};
	P=$$(realpath .); \
	BOARDS_TXT=$(shell grep -l '$(BRD).name=' $(BOARDS_FILES));\
	cd $$(dirname $<); \
		make -f $$P/arduino.debian.mk \
			ARDUINO_DIR=$(ARDUINO_DIR) \
			BOARDS_TXT=$$BOARDS_TXT \
			BOARD_TAG=$(BRD) \
			ARDUINO_PORT=$(call key,PORTS,$(PRT)) \
			ARDUINO_LIBS=$(ARDUINO_LIBS) \
			$(ARDUINO_TARGETS);
	[ -e "$@" ] && { \
		$(ll) ok "$@" "Done"; \
	} || {  \
		$(ll) error "$@" "Missing target"; \
		exit 1; \
	};
endef

$/%.hex: $/%.ino
	@\
	$(build-arduino-firmware)

$/%.hex: $/%.pde
	@\
	$(build-arduino-firmware)

Mpe/CarrierCase/CarrierCase.hex: BRD := uno

carriercase: BRD := uno
carriercase: P := Mpe/CarrierCase/
carriercase: I := Mpe/CarrierCase/CarrierCase.hex
carriercase: jeenode upload


# XXX: how does Mpe/MySketch/MySketch.hex get to firmware/Mpe-MySketch-<version>-<static-params>-<board>.hex
# Or how does some rule or one like this trigger the build and rename of
# Mpe/MySketch?
#firmware/Mpe-CarrierCase-uno-DEBUG.hex: DEBUG_BUILD := 1

# via params perhaps
#firmware/Mpe-CarrierCase.builds
# XXX: look at passing env vars

#      ------------ --
#
# vim:noet:
