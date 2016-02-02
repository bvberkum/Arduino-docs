# Arduino-mpe/Rules.mk
$(module-header,old,$/Rules.old.mk)
#
#      ------------ -- 

# XXX: makefile broke with Arduino 1.6.*
#
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

# avrdude chip ID
upload: C := m328p
# avrdude protocol
upload: M := arduino
# flash image
#upload: I := firmware/ArduinoISP_mega328.hex
#upload: I := firmware/isp_flash_m328p.hex
# avrdude extra flags
upload: X := -D
upload: _upload

avrdude=$(ARDUINODIR)/hardware/tools/avr/bin/avrdude -C$(ARDUINODIR)/hardware/tools/avr/etc/avrdude.conf

_upload:
	@\
	$(ll) attention $@ "Starting upload to $(C) using $(M).." $(I);\
	[ "$(M)" = "usbasp" ] && { sudo="sudo "; } || { sudo=; }; \
	$(call _flash)
	@\
	$(ll) OK $@ "Flash upload completed successfully"

download: C := m328p
download: M := arduino
download: I := 
download: X := -D
download: _download


_read_fuses:
	$(avrdude) \
		-p $(C) \
		$(call key,METHODS,$(M)) \
		-U lock:r:-:h -U lfuse:r:-:h -U hfuse:r:-:h

_download:
	@\
	$(ll) attention $@ "Starting flash/eeprom download using $(M).." $(I);\
	[ "$(M)" = "usbasp" ] && { sudo="sudo "; } || { sudo=; }; \
	I=$(I);\
		[ -z "$$I" ] && I=download-$(C)-$(M);\
	$(avrdude) \
		-p $(C) \
		$(call key,METHODS,$(M)) \
		-U eeprom:r:$$I-eeprom.hex:i \
		-U flash:r:$$I.hex:i \
		-U lock:r:-:h -U lfuse:r:-:h -U hfuse:r:-:h
	@\
	$(ll) OK $@ "Download completed successfully" $$I-*

_uctest:
	@\
	$(ll) attention $@ "Testing for $(C) using $(M).." $(PORT);\
	[ "$(M)" = "usbasp" ] && { sudo="sudo "; } || { sudo=; }; \
	$(D) $${sudo}$(avrdude) \
		-p $(C) \
		$(call key,METHODS,$(M)) \
		-U lock:r:-:h -U lfuse:r:-:h -U hfuse:r:-:h \
		$(X)
	@$(ll) OK $@

uctest: C := m328p
uctest: M := usbasp
uctest: X := 
uctest: _uctest

m328ptest: C := m328p
m328ptest: M := arduinoisp
m328ptest: X := 
m328ptest: _uctest

t85test: C := t85
t85test: M := usbasp
t85test: X := "-B3"
t85test: _uctest

erase: C := m328p
erase: M := arduinoisp
erase:
	@\
	$(ll) attention $@ "Starting erase of $(C) using $(M)..";\
	$(avrdude) \
		-p $(C) $(call key,METHODS,$(M)) -e;\
	$(ll) OK $@ 

flash: C := m8
flash: M := usbasp
flash: I := 
# Low-, high- and extended fuse
flash: LF := 
flash: HF := 
flash: EF := 
# Lock/unlock bits
flash: LB := 
flash: UB := 
# Debug
flash: D := echo
flash: _flash

_flash:
	@\
	[ "$(M)" = "usbasp" ] && { sudo="sudo "; } || { sudo=; }; \
	$(ll) attention $@ "Starting flash to $(C) using $(M).." $(I);\
	X="$(X)";\
	( [ -n "$(HF)" ] && [ -n "$(LF)" ] || { exit 1; } ) && ( \
		[ -n "$(UB)" ] && { \
			$(ll) attention $@ "Unlocking & erasing.." && \
			$(D) $${sudo}$(avrdude) \
				-p $(C) -e $$X \
				$(call key,METHODS,$(M)) \
				-U lock:w:$(UB):m \
				|| exit 2;\
		}; \
		$(ll) attention $@ "Writing new fuses.." && \
		([ -n "$(EF)" ] && { \
			$(D) $${sudo}$(avrdude) \
				-p $(C) -e $$X \
				$(call key,METHODS,$(M)) \
				-U hfuse:w:$(HF):m \
				-U lfuse:w:$(LF):m \
				-U efuse:w:$(EF):m \
				&& $(ll) info $@ OK \
				|| exit 3;\
		} || { \
			$(D) $${sudo}$(avrdude) -p $(C) -e $$X \
				$(call key,METHODS,$(M)) \
				-U hfuse:w:$(HF):m \
				-U lfuse:w:$(LF):m \
				&& $(ll) info $@ OK \
				|| exit 4;\
		});\
		([ -n "$(E)" ] && { \
			$(ll) attention $@ "Writing EEPROM.." && \
			$(D) $${sudo}$(avrdude) -p $(C) -D $$X \
				$(call key,METHODS,$(M)) \
				-U eeprom:w:$(E) \
				&& $(ll) info $@ OK \
				|| exit 5;\
		});\
		([ -n "$(I)" ] && { \
			$(ll) attention $@ "Writing Flash.." && \
			$(D) $${sudo}$(avrdude) -p $(C) -D $$X \
				$(call key,METHODS,$(M)) \
				-V -U flash:w:$(I) \
				&& $(ll) info $@ OK \
				|| exit 6;\
		});\
		([ -n "$(LB)" ] && { \
			$(ll) attention $@ "Locking.." && \
				$(D) $${sudo}$(avrdude) $$X \
					-p $(C) -D \
					$(call key,METHODS,$(M)) \
					-U lock:w:$(LB):m \
					&& $(ll) info $@ OK \
					|| exit 7;\
		}); \
	)

### Preset common flash 

# 1st version 
# Update 3: Baud: 2400
# Update 4: Baud: 4800
fusebit-doctor-m328-update8: C := m328p
fusebit-doctor-m328-update8: M := usbasp
fusebit-doctor-m328-update8: I := $(wildcard ~/Downloads/atmega-hvpp-fusebit-doctor_archive/update8-05.03.2011/firmware/Atmega328,328P/atmega_fusebit_doctor_2.08_m328p.hex)
fusebit-doctor-m328-update8: E := $(wildcard ~/Downloads/atmega-hvpp-fusebit-doctor_archive/update8-05.03.2011/firmware/Atmega328,328P/atmega_fusebit_doctor_2.08_m328p.bin)
fusebit-doctor-m328-update8: LF := 0x62
fusebit-doctor-m328-update8: HF := 0xD1
fusebit-doctor-m328-update8: EF := 0x07
fusebit-doctor-m328-update8: LB := 0x0F
fusebit-doctor-m328-update8: UB := 0x3F
fusebit-doctor-m328-update8: X := -B 3
fusebit-doctor-m328-update8: _flash

fusebit-doctor-m328-update10: C := m328p
fusebit-doctor-m328-update10: M := usbasp
fusebit-doctor-m328-update10: I := $(wildcard ~/Downloads/atmega-hvpp-fusebit-doctor_archive/update10-20.04.2011/firmware/Atmega328,328P/atmega_fusebit_doctor_2.10_m328p.hex)
fusebit-doctor-m328-update10: E := $(wildcard ~/Downloads/atmega-hvpp-fusebit-doctor_archive/update10-20.04.2011/firmware/Atmega328,328P/atmega_fusebit_doctor_2.10_m328p.bin)
#fusebit-doctor-m328: I := firmware/atmega_fusebit_doctor_2.11_m328p.hex
#fusebit-doctor-m328: LF := 0x62
fusebit-doctor-m328-update10: LF := 0x62
fusebit-doctor-m328-update10: HF := 0xD1
fusebit-doctor-m328-update10: EF := 0x07
fusebit-doctor-m328-update10: LB := 0x0F
fusebit-doctor-m328-update10: UB := 0x3F
fusebit-doctor-m328-update10: X := -B 3
fusebit-doctor-m328-update10: _flash
#fusebit-doctor-m328: D := 

fusebit-doctor-m328-update11: C := m328p
fusebit-doctor-m328-update11: M := usbasp
fusebit-doctor-m328-update11: I := $(wildcard ~/Downloads/atmega-hvpp-fusebit-doctor_archive/update11-30.04.2011/firmware/Atmega328,328P/atmega_fusebit_doctor_2.11_m328p.hex)
fusebit-doctor-m328-update11: E := $(wildcard ~/Downloads/atmega-hvpp-fusebit-doctor_archive/update11-30.04.2011/firmware/Atmega328,328P/atmega_fusebit_doctor_2.11_m328p.bin)
fusebit-doctor-m328-update11: LF := 0x62
fusebit-doctor-m328-update11: HF := 0xD1
fusebit-doctor-m328-update11: EF := 0x07
fusebit-doctor-m328-update11: LB := 0x0F
fusebit-doctor-m328-update11: UB := 0x3F
fusebit-doctor-m328-update11: X := -B 3
fusebit-doctor-m328-update11: _flash

fusebit-doctor: fusebit-doctor-m328-update10

fusebit-doctor-m8: C := m8
fusebit-doctor-m8: M := usbasp
fusebit-doctor-m8: I := atmega_fusebit_doctor_2.09_m8.hex
fusebit-doctor-m8: LF := 0xE1
fusebit-doctor-m8: HF := 0xD1
fusebit-doctor-m8: EF :=
fusebit-doctor-m8: LB := 0x3C
fusebit-doctor-m8: UB := 0x3F
fusebit-doctor-m8: D := 
fusebit-doctor-m8: _flash


upload-betemcu: C := m8
upload-betemcu: M := usbasp
upload-betemcu: I := 
upload-betemcu: _flash

flash-betemcu-usbasploader: C := m8
flash-betemcu-usbasploader: M := usbasp
flash-betemcu-usbasploader: I := firmware/betemcu-usbasp/usbasp.2011-05-28/bin/firmware/usbasp.atmega8.2011-05-28.hex
flash-betemcu-usbasploader: LF := 0xFF
flash-betemcu-usbasploader: HF := 0xD9
flash-betemcu-usbasploader: EF := 
flash-betemcu-usbasploader: LB := 0x3C
flash-betemcu-usbasploader: UB := 0x3F
flash-betemcu-usbasploader: _flash

flash-betemcu-usbasploader-alt: M := usbasp
flash-betemcu-usbasploader-alt: I := firmware/betemcu-usbasp/alternate_USBaspLoader_betemcu_timeout.hex
flash-betemcu-usbasploader-alt:
	$(avrdude) \
		-p m8 -e \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x3F:m
	$(avrdude) \
		-p m8 \
		$(call key,METHODS,$(M)) \
		-U hfuse:w:0xC8:m -U lfuse:w:0xBF:m
	$(avrdude) \
		-p m8 \
		$(call key,METHODS,$(M)) \
		-U flash:w:$(I)
	$(avrdude) \
		-p m8 \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x0F:m

# from /home/berend/Application/arduino-0021/hardware/arduino/boards.txt
# This is working with 19200 baud
m8-16MHz: C := m8
m8-16MHz: M := usbasp
#m8-16MHz: I := firmware/ATmegaBOOT.hex
# from 1.0.3
m8-16MHz: I := firmware/ATmegaBOOT-prod-firmware-2009-11-07.hex
m8-16MHz: HF := 0xCA
m8-16MHz: LF := 0xDF
m8-16MHz: LB := 0x0F
m8-16MHz: UB := 0x3F
m8-16MHz: X := -B 3 
m8-16MHz: _flash

# not sure what this is
m8-fd: C := m8
m8-fd: M := usbasp
m8-fd: HF := 0x99
m8-fd: LF := 0XC1
m8-fd: LB := 0x0F
m8-fd: UB := 0x3F
m8-fd: X := -B 3 
m8-fd: _flash

# 8MHz optiboot, cannot get this to work at 9600b
# http://www.robertoinzerillo.com/wordpress/wp-content/uploads/2012/10/optiboot_atmega8_8.zip
# http://www.robertoinzerillo.com/wordpress/?p=45
# Arduino BOARD: atmega8_opti_8mhz
m8-optiboot: C := m8
m8-optiboot: M := usbasp
m8-optiboot: HF := 0xCC
m8-optiboot: LF := 0xA4
m8-optiboot: LB := 0x0F
m8-optiboot: UB := 0x3F
m8-optiboot: I := firmware/optiboot_atmega8_8.hex
m8-optiboot: X := -B 3 
m8-optiboot: D := 
m8-optiboot: _flash

# 8MHz noxtal
# http://todbot.com/blog/2009/05/26/minimal-arduino-with-atmega8/
# http://todbot.com/blog/wp-content/uploads/2009/05/atmega8_noxtal.zip
# Arduino BOARD: atmega8noxtal
m8-noxtal: C := m8
m8-noxtal: M := usbasp
m8-noxtal: HF := 0xC4
m8-noxtal: LF := 0xE4
m8-noxtal: LB := 0x0F
m8-noxtal: UB := 0x3F
m8-noxtal: I := firmware/atmega8_noxtal/ATmegaBOOT.hex
#m8-noxtal: X := -B 3 
m8-noxtal: D := 
m8-noxtal: _flash

m8: M := arduinoisp
m8: 
	avrdude \
		-p m8 \
		$(call key,METHODS,$(M)) 

m16: M := arduinoisp
m16: 
	avrdude \
		-p m16 \
		$(call key,METHODS,$(M)) 

m32-DI-8MHz-int: M := arduinoisp
m32-DI-8MHz-int: I := firmware/ATmegaBOOT_168_diecimila.hex
m32-DI-8MHz-int:
	avrdude \
		-p m32 -e \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x3F:m -U lfuse:w:0xD4:m -U hfuse:w:0x99:m
	avrdude \
		-p m32 \
		$(call key,METHODS,$(M)) \
		-D -U flash:w:$(I)
	avrdude \
		-p m32 \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x0F:m

# 16MHz ext, 5V, delay bootloader
m32: M := arduinoisp
m32: I := firmware/ATmegaBOOT_168_diecimila.hex
m32:
	avrdude \
		-p m32 -e \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x3F:m -U lfuse:w:0xD4:m -U hfuse:w:0x99:m
	avrdude \
		-p m32 \
		$(call key,METHODS,$(M)) \
		-D -U flash:w:$(I)
	avrdude \
		-p m32 \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x0F:m

m48: M := arduinoisp
m48: 
	avrdude \
		-p m48 \
		$(call key,METHODS,$(M)) 

m328p: M := arduinoisp
m328p: 
	avrdude \
		-p m328p \
		$(call key,METHODS,$(M)) 

# Internal 8MHz/57600baud 328 target
m328p-8MHz: I := firmware/ATmegaBOOT_168_atmega328_pro_8MHz.hex
m328p-8MHz: M := arduinoisp
m328p-8MHz:
	avrdude \
		-p m328p -e \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x3F:m -U lfuse:w:0xE2:m -U hfuse:w:0xDA:m -U efuse:w:0x05:m
	avrdude \
		-p m328p \
		$(call key,METHODS,$(M)) \
		-D -U flash:w:$(I)
	avrdude \
		-p m328p \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x0F:m

#m328p-16MHz: I := firmware/optiboot_atmega328.hex

m328p-16MHz: I := firmware/ATmegaBOOT_168_atmega328.hex
m328p-16MHz: M := usbasp
m328p-16MHz: X :=
m328p-16MHz: C := m328p
m328p-16MHz: LF := 0xFF
m328p-16MHz: HF := 0xDA
m328p-16MHz: EF := 0x07
m328p-16MHz: LB := 0x0F
m328p-16MHz: UB := 0x3F
m328p-16MHz: _flash

# Cannot re-read protected flash without -e? check fuses
#verify-betemcu: M := usbasp
#verify-betemcu:
#	sudo avrdude -p m8 \
#		$(call key,METHODS,$(M)) \
#  	-U lock:w:0x3F:m 
#	sudo avrdude -p m8 -v \
#		$(call key,METHODS,$(M)) \
#		-U flash:v:firmware/betemcu-usbasp/usbasp.2011-05-28/bin/firmware/usbasp.atmega8.2011-05-28.hex:i
#		-U lock:v:0x3C:m -U lfuse:v:0xff:m -U hfuse:v:0xd9:m
#	sudo avrdude -p m8 \
#		$(call key,METHODS,$(M)) \
#  	-U lock:w:0x3C:m 


m1284p: M := arduinoisp
m1284p: 
	avrdude \
		-p m1284p \
		$(call key,METHODS,$(M)) 

#      ------------ -- 
# 
# Integrating with another makefile for easy builds

#ARDUINODIR := /home/berend/Application/arduino-1.0.3
#ARDUINODIR := ./tmp/arduino-1.0.4
#ARDUINODIR := /usr/share/arduino/
#ARDUINODIR := /home/berend/Application/arduino-1.0.3
#ARDUINODIR := /usr/share/arduino/
#ARDUINODIR := $(shell realpath ./arduino-1.0.5)
#ARDUINODIR := $(shell realpath ./arduino-1.0.1)

#AVRTOOLSPATH += $(ARDUINODIR)/hardware/tools
#AVRTOOLSPATH += $(ARDUINODIR)/hardware/tools/avr/bin

#$(info $(shell $(ll) header1 $(MK_$d) Wrapping arduino.mk))
#$(info $(shell $(ll) header3 $(MK_$d) AVRTOOLSPATH:  $(AVRTOOLSPATH)))
#$(info $(shell $(ll) header3 $(MK_$d) ARDUINODIR: $(ARDUINODIR)))
#
#BOARDS_FILES := $(wildcard \
#	$(ARDUINODIR)/hardware/*/boards.txt \
#	./hardware/*/boards.txt)
#$(info BOARDS_FILES = $(BOARDS_FILES))
#
#BOARD_BUILD_MCU := $(foreach BOARDS_FILE,$(BOARDS_FILES),\
#	$(shell sed -ne "s/$(BOARD).build.mcu=\(.*\)/\1/p" $(BOARDS_FILE)))
#$(info BOARD_BUILD_MCU = $(BOARD_BUILD_MCU))

# Build anything in target folder 'P'
#arduino: P :=
#arduino: BRD := 
#_arduino: LIB := $/libraries/
#_arduino: TARGETS := clean all
_arduino:
	@\
	$(call log,header2,BRD,$(BRD));\
	p=$$(realpath .);\
	cd $P; \
		ARDUINODIR=$(ARDUINODIR) \
		AVRTOOLSPATH="$(AVRTOOLSPATH)" \
		BOARD=$(BRD) \
		make -f $$p/arduino.mk $(TARGETS)

arduino: TARGETS := target
arduino: _arduino

jeenode: BRD := atmega328
jeenode: M := arduino
jeenode: arduino

arduino-firmware: BRD := 
arduino-firmware: P := 
arduino-firmware: TARGETS := 
arduino-firmware: _arduino-firmware

_arduino-firmware:
	$(call log,header2,BRD,$(BRD));\
	p=$$(realpath .);\
	cd $P; \
	make -f $$p/arduino.mk \
		ARDUINODIR=$(ARDUINODIR) \
		BOARD=$(BRD) \
		$(TARGETS)

boards: TARGETS := boards
boards: _arduino

monitor: TARGETS := monitor
monitor: _arduino

size: P := 
size: TARGETS := size
size: _arduino


ardnlib:
	cd $(ARDUINODIR)/libraries/;\
	ln -s /src/jeelabs/jeelib JeeLib; \
	ln -s /src/jeelabs/embencode EmBencode; \
	ln -s /src/jeelabs/ethercard EtherCard; \
	ln -s /srv/project-mpe/Arduino-mpe/libraries/DHT; \
	ln -s /srv/project-mpe/Arduino-mpe/libraries/OneWire

isp_flash: P := $/libraries/JeeLib/examples/Ports/isp_flash/
isp_flash: I := $/libraries/JeeLib/examples/Ports/isp_flash/isp_flash.hex
isp_flash: BRD := atmega328
isp_flash: arduino upload

arduino-boards:
	@p=$$(realpath .);\
	echo ARDUINODIR=$(ARDUINODIR);\
	make -f $$p/arduino.mk boards

### Common preset uploads

jeenodeisp: I := firmware/isp_flash_m328p.hex
jeenodeisp: X := -D
jeenodeisp: upload

#jeenode-isp-repair: P = $/libraries/JeeLib/examples/Ports/isp_repair2/
#jeenode-isp-repair: arduino
#
#jeenode-isp-repair-post: DIR := $/
#jeenode-isp-repair-post:
#	cp $(DIR)libraries/JeeLib/examples/Ports/isp_repair2/isp_repair2.hex firmware/isp_repair_m328p.hex

jeenodeisp-repair: C := m328p
jeenodeisp-repair: M := arduino
jeenodeisp-repair: I := firmware/isp_repair2_m328p.hex
#jeenodeisp-repair: X := -D
jeenodeisp-repair: upload

blink: C := m328p
blink: P := Prototype/Blink
blink: I := Prototype/Blink/Blink.hex
blink: jeenode upload

blinknodelayJeelib: C := m328p
blinknodelayJeelib: P := Prototype/Blink/BlinkNodelayJeelib
blinknodelayJeelib: I := Prototype/Blink/BlinkNodelayJeelib/BlinkNodelayJeelib.hex
blinknodelayJeelib: jeenode upload

blinkall: C := m328p
blinkall: P := Prototype/Blink/BlinkAll
blinkall: I := Prototype/Blink/BlinkAll/BlinkAll.hex
blinkall: jeenode upload

oe: C := m328p
oe: P := Mpe/Plugs/OutputExpander
oe: I := Mpe/Plugs/OutputExpander/OutputExpander.hex
oe: jeenode upload

3way: C := m328p
3way: P := Mpe/eBay-ThreeWayMeter/
3way: I := Mpe/eBay-ThreeWayMeter/Prototype.hex
3way: jeenode upload

radioblip: C := m328p
radioblip: P := Mpe/RadioBlip/
radioblip: I := Mpe/RadioBlip/RadioBlip.hex
radioblip: jeenode upload

# XXX merge with Prototype if needed
#radiolink: C := m328p
#radiolink: P := Mpe/RadioLink/
#radiolink: I := Mpe/RadioLink/RadioLink.hex
#radiolink: jeenode upload

radiolink: C := m328p
radiolink: P := Prototype/RadioLink/
radiolink: I := Prototype/RadioLink/RadioLink.hex
radiolink: jeenode upload

# RF24 Compiles with 1.0.1,4,5 on darwin. Latest libs.
rf24helper: C := m328p
rf24helper: P := Mpe/RF24Helper
rf24helper: I := Mpe/RF24Helper/RF24Helper.hex
rf24helper: jeenode upload

rf24test: C := m328p
rf24test: P := Mpe/RF24Test
rf24test: I := Mpe/RF24Test/RF24Test.hex
rf24test: jeenode upload

rf24scan: C := m328p
rf24scan: P := libraries/RF24/examples/scanner/
rf24scan: I := libraries/RF24/examples/scanner/scanner.hex
rf24scan: jeenode upload

rf24ping: C := m328p
rf24ping: P := libraries/RF24/examples/pingpair/
rf24ping: I := libraries/RF24/examples/pingpair/pingpair.hex
rf24ping: jeenode upload

# RF24Network Compiles with 1.0.4 or 1.0.5 on Darwin
rf24hwtx: C := m328p
#rf24hwtx: BRD := atmega8
rf24hwtx: P := libraries/RF24Network/examples/helloworld_tx/
rf24hwtx: I := libraries/RF24Network/examples/helloworld_tx/helloworld_tx.hex
#rf24hwtx: arduino _upload
rf24hwtx: jeenode upload

#rf24hwrx: C := m8
#rf24hwrx: BRD := atmega8
rf24hwrx: P := libraries/RF24Network/examples/helloworld_rx/
rf24hwrx: I := libraries/RF24Network/examples/helloworld_rx/helloworld_rx.hex
rf24hwrx: jeenode upload

sensornet: P := libraries/RF24Network/examples/sensornet/
sensornet: I := libraries/RF24Network/examples/sensornet/sensornet.hex
sensornet: jeenode upload

meshping: P := libraries/RF24Network/examples/meshping/
meshping: I := libraries/RF24Network/examples/meshping/meshping.hex
meshping: jeenode upload

carriercase: C := m328p
#carriercase: BRD := uno
carriercase: P := Mpe/CarrierCase/
carriercase: I := Mpe/CarrierCase/CarrierCase.hex
carriercase: jeenode upload

cassette328p: C := m328p
cassette328p: P := Mpe/Cassette328P/
cassette328p: I := Mpe/Cassette328P/Cassette328P.hex
cassette328p: jeenode upload

cassette328pmilli: C := m328p
cassette328pmilli: P := Mpe/Cassette328P/Milli/
cassette328pmilli: I := Mpe/Cassette328P/Milli/Milli.hex
cassette328pmilli: jeenode upload

hanrun: C := m328p
hanrun: P := Misc/HanrunENC28J60/
hanrun: I := Misc/HanrunENC28J60/HanrunENC28J60.hex
hanrun: jeenode upload

utilitybug: C := m328p
utilitybug: P := Mpe/Utility/UtilityBug/
utilitybug: I := Mpe/Utility/UtilityBug/UtilityBug.hex
utilitybug: jeenode upload

gastest: C := m328p
gastest: P := Mpe/GasDetector/Test/
gastest: I := Mpe/GasDetector/Test/Test.hex
gastest: jeenode upload

gasdet: C := m328p
gasdet: P := Mpe/GasDetector/
gasdet: I := Mpe/GasDetector/GasDetector.hex
gasdet: jeenode upload

sandbox: C := m328p
sandbox: P := Mpe/Sandbox/
sandbox: I := Mpe/Sandbox/Sandbox.hex
sandbox: jeenode upload

sandbox-avr: C := m328p
sandbox-avr: P := Mpe/Sandbox/AVR
sandbox-avr: I := Mpe/Sandbox/AVR/AVR.hex
sandbox-avr: arduino _upload

mla: C := m328p
mla: P := Misc/miniLogicAnalyzer/
mla: I := Misc/miniLogicAnalyzer/miniLogicAnalyzer.hex
mla: jeenode upload

logicanalyzer: C := m328p
logicanalyzer: P := Mpe/LogicAnalyzer/
logicanalyzer: I := Mpe/LogicAnalyzer/LogicAnalyzer.hex
logicanalyzer: jeenode upload

pcd8544: C := m328p
pcd8544: P := Misc/nokia_pcd8544_display_testing2/
pcd8544: I := Misc/nokia_pcd8544_display_testing2/nokia_pcd8544_display_testing2.hex
pcd8544: jeenode upload

ledseg: C := m328p
ledseg: P := libraries/LedControl/examples/LCDemo7Segment
ledseg: I := libraries/LedControl/examples/LCDemo7Segment/LCDemo7Segment.hex
ledseg: jeenode upload

ledseg595: C := m328p
ledseg595: P := Mpe/LedSegment595/
ledseg595: I := Mpe/LedSegment595/LedSegment595.hex
ledseg595: jeenode upload

# See ~/project/transistortester
avrtransistortester: E := firmware/TransistorTester.eep
avrtransistortester: M := usbasp
avrtransistortester: X := -B 3
avrtransistortester: _flash

avrtransistortester168: I := firmware/TransistorTester_168.hex
avrtransistortester168: LF := 0xE2
avrtransistortester168: HF := 0xDC
avrtransistortester168: UB := 0x3F
avrtransistortester168: LB := 0x30
avrtransistortester168: C := m168p
avrtransistortester168: avrtransistortester

avrtransistortester328: I := $(wildcard ~/project/transistortester/Software/trunk/mega328_3.3V/TransistorTester.hex)
avrtransistortester328: E := $(wildcard ~/project/transistortester/Software/trunk/mega328_3.3V/TransistorTester.eep)
avrtransistortester328: LF := 0xE2
avrtransistortester328: HF := 0xDC
	# 04 is same as FC, E4, etc
avrtransistortester328: EF := 0x04
avrtransistortester328: UB := 
avrtransistortester328: LB := 
avrtransistortester328: C := m328p
avrtransistortester328: M := usbasp
avrtransistortester328: X := -B 3
avrtransistortester328: _flash

#avrtransistortester: E := Misc/AVR-Transistortester_neu/ATmega8/TransistorTestNew.eep
#avrtransistortester: I := Misc/AVR-Transistortester_neu/ATmega8/TransistorTestNew.hex
# XXX: This one gives timeout, circuit change?
#avrtransistortester: E := Mpe/transistortester/Software/trunk/mega8/TransistorTester.eep
#avrtransistortester: I := Mpe/transistortester/Software/trunk/mega8/TransistorTester.hex


at85blink: M:=usbasp
at85blink: BRD:=t85
at85blink: LF:=0x62
at85blink: HF:=0xD9
at85blink: C:=t85
at85blink: X:=-B1
at85blink: P:=Prototype/Blink
at85blink: I:=Prototype/Blink/Blink.hex
at85blink: TARGETS:= clean all
at85blink: _arduino _flash

# micronucleus allows v-usb uploads
at85mn: M:=usbasp
at85mn: C:=t85
at85mn: LF:=0xE1
at85mn: HF:=0xDD
at85mn: EF:=0xFE
at85mn: X:=-B1
at85mn: I:=firmware/attiny85-micronucleus-bootloader.hex
at85mn: _flash

at85usb: M:=usbasp
at85usb: BRD:=t85
at85usb: C:=t85
at85usb: X:=-B 3
#-q -q
#no locking:
at85usb: LB:= 
at85usb: UB:= 
#v-usb atiny fuses:
at85usb: LF:=0xE1
at85usb: HF:=0xDD
#clock out PB4:
#at85usb: LF:=0xA1
#at85usb: HF:=0xDD
#int 1MHz:
#at85usb: LF:=0x62
#at85usb: HF:=0xDF
#clock out PB4:
#at85usb: LF:=0x22
#at85usb: HF:=0xDF
#at85usb: D:=echo
#at85usb: P:=Mpe/BlinkAll/
#at85usb: I:=Mpe/BlinkAll/BlinkAll.hex
#at85usb: TARGETS := clean all
#at85usb: _arduino _flash
at85usb: P:=Mpe/TinyUSB/TinyUSBLedDimmer1
at85usb: I:=Mpe/TinyUSB/TinyUSBLedDimmer1/TinyUSBLedDimmer1.hex
at85usb: TARGETS := clean all
at85usb: _arduino _flash

#at85usb: I:=Misc/usb_tiny85/main.hex
#at85usb: _flash

lcd1602_i2c: C := m328p
lcd1602_i2c: BRD := atmega328
# this is for JeeLabs expander plug
#lcd1602_i2c: P := Mpe/LCD_I2C
#lcd1602_i2c: I := Mpe/LCD_I2C/LCD_I2C.hex
lcd1602_i2c: P := Misc/I2C_LCD_CustomChars_mjkdz
lcd1602_i2c: I := Misc/I2C_LCD_CustomChars_mjkdz/I2C_LCD_CustomChars_mjkdz.hex
lcd1602_i2c: _arduino upload

i2cpir: C := m328p
i2cpir: P := Mpe/I2CPIR/
i2cpir: I := Mpe/I2CPIR/I2CPIR.hex
i2cpir: jeenode upload

all_i2c: C := m328p
all_i2c: BRD := atmega328
all_i2c: P := Mpe/i2c_all/
all_i2c: I := Mpe/i2c_all/i2c_all.hex
all_i2c: _arduino upload

I2CLCD: C := m328p
I2CLCD: P := Prototype/I2CLCD/
I2CLCD: I := Prototype/I2CLCD/I2CLCD.hex
I2CLCD: jeenode upload

rf12mon: C := m328p
rf12mon: P := Misc/rf12mon/
rf12mon: I := Misc/rf12mon/rf12mon.hex
rf12mon: jeenode upload

rf12bert: C := m328p
rf12bert: P := Misc/rf12bert/
rf12bert: I := Misc/rf12bert/rf12bert.hex
rf12bert: jeenode upload

nrfmon: C := m328p
nrfmon: P := Mpe/Milli/
nrfmon: I := Mpe/Milli/Milli.hex
nrfmon: jeenode upload

mbug1: C := m328p
mbug1: P := Mpe/
mbug1: I := Mpe/MoistureBug/MoistureNode/MoistureNode.hex
mbug1: jeenode upload

soarer: I := Misc/Soarer_Converter/firmware/Soarer_at2usb_v1.12_atmega32u4.hex
soarer: M := leonardo
soarer: C := atmega32u4
#soarer: PORT := /dev/ttyACM0
soarer: PORT := $(wildcard /dev/tty.usbmodem*)
soarer: X := -D -v
soarer: SC := samsung
soarer: _upload sc-$(SC)

sc-samsung::
	./Misc/Soarer_Converter/tools/scas \
			./Misc/Soarer_Converter/samsung-SEM-M2A.sc \
			./Misc/Soarer_Converter/samsung.bin
	./Misc/Soarer_Converter/tools/scwr ./Misc/Soarer_Converter/samsung.bin

jeedht: C := m328p
jeedht: P := Prototype/DHTTest/JeeLibDHT/
jeedht: I := Prototype/DHTTest/JeeLibDHT/JeeLibDHT.hex
jeedht: jeenode upload

adadht: C := m328p
adadht: P := Prototype/DHTTest/AdafruitDHT/
adadht: I := Prototype/DHTTest/AdafruitDHT/AdafruitDHT.hex
adadht: jeenode upload

pcdthermotest: C := m328p
pcdthermotest: P := Misc/PCD8544_Thermometer/
pcdthermotest: I := Misc/PCD8544_Thermometer/PCD8544_Thermometer.hex
pcdthermotest: jeenode upload

m8guard: C := m8
m8guard: BRD := atmega8
m8guard: M := usbasp
m8guard: X := -B3
m8guard: P := Prototype/TempGuard/Atmega8TempGuard/
m8guard: I := Prototype/TempGuard/Atmega8TempGuard/Atmega8TempGuard.hex
#m8guard: TARGETS := clean all
m8guard: arduino _upload

#dstbus: C := m8
#dstbus: BRD := atmega8
#dstbus: C := m328p
#dstbus: BRD := atmega328
#dstbus: M := usbasp
#dstbus: X := -B3
dstbus: P := Prototype/DallasTempBus/
dstbus: I := Prototype/DallasTempBus/DallasTempBus.hex
dstbus: jeenode upload

lcd5110: C := m328p
lcd5110: P := Prototype/Lcd84x48/
lcd5110: I := Prototype/Lcd84x48/Lcd84x48.hex
lcd5110: jeenode upload

log5110: C := m328p
log5110: P := Prototype/LogReader84x48/
log5110: I := Prototype/LogReader84x48/LogReader84x48.hex
log5110: jeenode upload

thermo5110: C := m328p
thermo5110: P := Prototype/ThermoLog84x48/
thermo5110: I := Prototype/ThermoLog84x48/ThermoLog84x48.hex
thermo5110: jeenode upload

rf24link: C := m328p
rf24link: P := Prototype/RadioLinkRF24
rf24link: I := Prototype/RadioLinkRF24/RadioLinkRF24.hex
rf24link: jeenode upload

rf24node: C := m328p
rf24node: P := Prototype/RF24Node
rf24node: I := Prototype/RF24Node/RF24Node.hex
rf24node: jeenode upload

eeprom: C := m328p
eeprom: P := Prototype/AtmegaEEPROM/
eeprom: I := Prototype/AtmegaEEPROM/AtmegaEEPROM.hex
eeprom: jeenode upload

attemp: C := m328p
attemp: P := Prototype/AtmegaTemp
attemp: I := Prototype/AtmegaTemp/AtmegaTemp.hex
attemp: arduino upload

mmcinfo: C := m328p
mmcinfo: P := Misc/MMC/CardInfo/
mmcinfo: I := Misc/MMC/CardInfo/CardInfo.hex
mmcinfo: jeenode upload

mmc: C := m328p
mmc: P := Prototype/MMC/
mmc: I := Prototype/MMC/MMC.hex
mmc: jeenode upload

jproomnode: C := m328p
jproomnode: P := libraries/JeeLib/examples/RF12/roomNode/
jproomnode: I := libraries/JeeLib/examples/RF12/roomNode/roomNode.hex
jproomnode: jeenode upload

roomnode: C := m328p
roomnode: P := Mpe/RoomNode/
roomnode: I := Mpe/RoomNode/RoomNode.hex
roomnode: jeenode upload

roomnode24: C := m328p
roomnode24: P := Mpe/RoomNodeRF24/
roomnode24: I := Mpe/RoomNodeRF24/RoomNodeRF24.hex
roomnode24: jeenode upload

magnetometer: C := m328p
magnetometer: P := Prototype/Magnetometer/
magnetometer: I := Prototype/Magnetometer/Magnetometer.hex
magnetometer: jeenode upload

# this needs some avr-gcc setup, not working
pff: C := m328p
pff: P := Prototype/MMC/PFF/
pff: I := Prototype/MMC/PFF/PFF.hex
pff: jeenode upload


### Prototypes

node: C := m328p
node: P := Prototype/Node/
node: I := Prototype/Node/Node.hex
node: jeenode upload

sensornode: C := m328p
sensornode: P := Prototype/SensorNode/
sensornode: I := Prototype/SensorNode/SensorNode.hex
sensornode: jeenode upload

serial: C := m328p
serial: P := Prototype/Serial/
serial: I := Prototype/Serial/Serial.hex
serial: jeenode upload

relaybox: C := m328p
relaybox: P := Prototype/RelayBox/
relaybox: I := Prototype/RelayBox/RelayBox.hex
relaybox: jeenode upload


### Tools

bandgap: C := m328p
bandgap: P := Tool/Bandgap/BandgapLefty/
bandgap: I := Tool/Bandgap/BandgapLefty/BandgapLefty.hex
bandgap: jeenode upload

bandgap_measure: C := m328p
bandgap_measure: P := Tool/Bandgap/BandgapMeasure/
bandgap_measure: I := Tool/Bandgap/BandgapMeasure/BandgapMeasure.hex
bandgap_measure: jeenode upload

bandgap_correct: C := m328p
bandgap_correct: P := Tool/Bandgap/BandgapCorrect/
bandgap_correct: I := Tool/Bandgap/BandgapCorrect/BandgapCorrect.hex
bandgap_correct: jeenode upload


### JeeLib

rf12demo: C := m328p
rf12demo: P := libraries/JeeLib/examples/RF12/RF12demo/
rf12demo: I := libraries/JeeLib/examples/RF12/RF12demo/RF12demo.hex
rf12demo: jeenode upload

jeeblip: C := m328p
jeeblip: P := libraries/JeeLib/examples/RF12/radioBlip/
jeeblip: I := libraries/JeeLib/examples/RF12/radioBlip/radioBlip.hex
jeeblip: jeenode upload

fuseboxmon: C := m328p
fuseboxmon: P := libraries/JeeLib/examples/RF12/p1scanner
fuseboxmon: I := libraries/JeeLib/examples/RF12/p1scanner/p1scanner.hex
fuseboxmon: jeenode upload

parser_demo: C := m328p
parser_demo: P := libraries/JeeLib/examples/Ports/parser_demo/
parser_demo: I := libraries/JeeLib/examples/Ports/parser_demo/parser_demo.hex
parser_demo: jeenode upload

jeebandgap: C := m328p
jeebandgap: P := libraries/JeeLib/examples/Ports/bandgap/
jeebandgap: I := libraries/JeeLib/examples/Ports/bandgap/bandgap.hex
jeebandgap: jeenode upload


TFT_ILI9163C: C := m328p
TFT_ILI9163C: P := Mpe/TFT_ILI9163C/
TFT_ILI9163C: I := Mpe/TFT_ILI9163C/TFT_ILI9163C.hex
TFT_ILI9163C: jeenode upload

TFT_ILI9163C_test: C := m328p
TFT_ILI9163C_test: P := libraries/TFT_ILI9163C/examples/test
TFT_ILI9163C_test: I := libraries/TFT_ILI9163C/examples/test/test.hex
TFT_ILI9163C_test: jeenode upload

TFT_ILI9163C_graphicstest: C := m328p
TFT_ILI9163C_graphicstest: P := libraries/TFT_ILI9163C/examples/graphicstest
TFT_ILI9163C_graphicstest: I := libraries/TFT_ILI9163C/examples/graphicstest/graphicstest.hex
TFT_ILI9163C_graphicstest: jeenode upload

habr-usbasp-i2c-tiny-usb: C := m8
habr-usbasp-i2c-tiny-usb: M := usbasp
habr-usbasp-i2c-tiny-usb: I := Misc/habr-usbasp/i2c_tiny_usb/i2c_tiny_usb.hex
habr-usbasp-i2c-tiny-usb: _upload

habr-usbasp-cdc-232: C := m8
habr-usbasp-cdc-232: M := usbasp
habr-usbasp-cdc-232: I := Misc/habr-usbasp/rs232/rs232.hex
habr-usbasp-cdc-232: _upload


ds1307-example: C := m328p
#ds1307-example: P := libraries/ds1307/examples/DS1307/
#ds1307-example: I := libraries/ds1307/examples/DS1307/DS1307.hex
ds1307-example: I := libraries/avr_lib_ds1307_01/src/ds1307/main.hex
ds1307-example: upload

# HID-Serial
#hid-example: C := m328p
#hid-example: P := libraries/HIDSerial/examples/echo/
#hid-example: I := libraries/HIDSerial/examples/echo/echo.hex
#hid-example: I := firmware/HID-Serial-hello_world.cpp.hex
hid-example: C := m8
hid-example: M := usbasp
hid-example: BRD := betemcu
hid-example: I := firmware/HID-Serial-echo.cpp.hex
hid-example: _upload


serialavail: C := m328
serialavail: BRD := atmega328
serialavail: P := Misc/Arduino/SerialAvailable/
serialavail: I := Misc/Arduino/SerialAvailable/SerialAvailable.hex
serialavail: jeenode _upload



pad1x4: C := m428p
pad1x4: M := arduino
pad1x4: P := Prototype/Keypad/Pad1x4/
pad1x4: I := Prototype/Keypad/Pad1x4/Pad1x4.hex
pad1x4: jeenode upload


### XXX old library symlinks, use submodules if possible

library: $(realpath $/libraries/)
	cd arduinodir/libraries/ \
		&& ~/bin/init-symlinks-relative.sh $<

#      ------------ -- 
#
# vim:noet:

