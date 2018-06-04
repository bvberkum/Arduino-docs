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
#


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
		-U lfuse:r:-:h \
		-U hfuse:r:-:h \
		-U lock:r:-:h \
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
	test -n "$(ERASE)" && erase_f=-e || erase_f=-D; \
	$(ll) attention $@ "Starting flash to $(C) using $(M).." $(I);\
	X="$(X)";\
	( [ -n "$(HF)" ] && [ -n "$(LF)" ] || { exit 1; } ) && ( \
		[ -n "$(UB)" ] && { \
			$(ll) attention $@ "Unlocking.." && \
			$(D) $${sudo}$(avrdude) \
				-p $(C) $$erase_f $$X \
				$(call key,METHODS,$(M)) \
				-U lock:w:$(UB):m \
				|| exit 2;\
		} || echo "No unlock"; \
		$(ll) attention $@ "Writing new fuses.." && \
		([ -n "$(EF)" ] && { \
			$(D) $${sudo}$(avrdude) \
				-p $(C) $$erase_f $$X \
				$(call key,METHODS,$(M)) \
				-U hfuse:w:$(HF):m \
				-U lfuse:w:$(LF):m \
				-U efuse:w:$(EF):m \
				&& $(ll) info $@ OK \
				|| exit 3;\
		} || { \
			$(D) $${sudo}$(avrdude) -p $(C) $$erase_f $$X \
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
		} || printf "");\
		([ -n "$(LB)" ] && { \
			$(ll) attention $@ "Locking.." && \
				$(D) $${sudo}$(avrdude) $$X \
					-p $(C) -D \
					$(call key,METHODS,$(M)) \
					-U lock:w:$(LB):m \
					&& $(ll) info $@ OK \
					|| exit 7;\
		} || printf ""); \
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
		#make -f /usr/local/opt/arduino-mk/Arduino.mk $(TARGETS)

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



### XXX old library symlinks, use submodules if possible

library: $(realpath $/libraries/)
	cd arduinodir/libraries/ \
		&& ~/bin/init-symlinks-relative.sh $<

#      ------------ -- 
#
# vim:noet:
