# add devices here, wildcard checks with FS for available devices
PORTS := $(wildcard /dev/tty.usbserial-* /dev/ttyUSB* /dev/tty.wchusbserial*)
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


avrdude=avrdude
#avrdude=$(ARDUINODIR)/hardware/tools/avr/bin/avrdude -C$(ARDUINODIR)/hardware/tools/avr/etc/avrdude.conf


METHODS = \
		arduino576="-c arduino -P $(PORT) -b 57600"; \
		arduino384="-c arduino -P $(PORT) -b 38400"; \
		arduino192="-c arduino -P $(PORT) -b 19200"; \
		arduino96="-c arduino -P $(PORT) -b 9600"; \
		arduino="-c arduino -P $(PORT) -b 57600"; \
		uno="-c arduino -P $(PORT) -b 115200"; \
		leonardo="-cavr109 -P $(PORT) -b 57600 "; \
		parisp="-c avr-par-isp-mpe -b 19200"; \
		parisp_="-c bsd -b 19200"; \
		arduino8="-cstk500 -P $(PORT) -b 19200"; \
		arduinoisp="-cstk500v1 -P $(PORT) -b 9600"; \
		arduinoisp_="-cstk500v1 -P $(PORT) -b 19200"; \
		arduinoisp__="-cstk500v1 -P $(PORT) -b 57600"; \
		usbasp="-c usbasp -P usb"
#IMAGES := \
#	blink=firmware/betemcu-usbasp/misc/betemcu_blink/betemcu_blink.cpp.hex\
#	ArduinoISP=ArduinoISP_mega328.hex
#atmega8_mkjdz.com_I2C_lcd1602.hex

#	ifneq ($(M),"parisp")
#	ifeq ("$(PORT)","")
#		$(error "No port: $(PORT)")
#	endif
#	endif
define _flash
	$(call log,header2,I,$(I));\
	$(call log,header2,C,$(C));\
	$(call log,header2,M,$(M));\
	$(call log,header2,X,$(X));\
	[ "$(M)" = "micronucleus" -o "$(M)" == "mn" ] && { \
		micronucleus $(I); \
	} || { \
		$(D) $${sudo}$(avrdude) \
			$(call key,METHODS,$(M))\
			-V \
			-p $(C) \
			-U flash:w:$(I) \
			$(X) ; \
	}
endef


_upload:
	@\
	$(ll) attention $@ "Starting upload to $(C) using $(M).." $(I);\
	[ "$(M)" = "usbasp" ] && { sudo="sudo "; } || { sudo=; }; \
	$(call _flash)
	@\
	$(ll) OK $@ "Flash upload completed successfully"

