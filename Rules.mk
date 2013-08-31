# Arduino-mpe/Rules.mk
include                $(MK_SHARE)Core/Main.dirstack.mk
MK_$d               := $/Rules.mk
MK                  += $(MK_$d)
#
#      ------------ -- 

#
#DIR                 := $/mydir
#include                $(call rules,$(DIR)/)
#
# DMK += $/dynamic-makefile.mk
# DEP += $/generated-dependency
# TRGT += $/build-target
# XXX: using find here is so wastefull, there is no cache at all?
CLN += $(shell find $/ -name .dep -or -name .lib)
# TEST += $/testtarget

METHODS = \
		arduino_="-c arduino -P $(PORT) -b 19200"; \
		arduino="-c arduino -P $(PORT) -b 57600"; \
		parisp="-c avr-par-isp-mpe -b 19200"; \
		arduinoisp="-cstk500v1 -P $(PORT) -b 9600"; \
		arduinoisp_="-cstk500v1 -P $(PORT) -b 19200"; \
		arduinoisp__="-cstk500v1 -P $(PORT) -b 57600"; \
	  usbasp="-c usbasp -P usb"
#IMAGES := \
#	blink=firmware/betemcu-usbasp/misc/betemcu_blink/betemcu_blink.cpp.hex\
#	ArduinoISP=ArduinoISP_mega328.hex
#atmega8_mkjdz.com_I2C_lcd1602.hex

ifeq ($(shell uname),"Linux")
PORT := /dev/tty.usbserial-A900TTH0
else
PORT := /dev/ttyUSB0
endif

find:
	find ./ -iname '*.hex'

listen: D := $(PORT)
listen: B := 57600
#listen: B := 38400
listen:
	minicom -D $(D) -b $(B) minirc.arduino

upload: C := m328p
upload: M := arduino
#upload: I := firmware/ArduinoISP_mega328.hex
#upload: I := firmware/isp_flash_m328p.hex
upload: X := -D
upload:
	avrdude \
		$(call key,METHODS,$(M))\
		-p $(C) \
		-U flash:w:$(I) \
		$(X)

download: C := m328p
download: M := arduino
download: I := 
download: X := -D
download:
	I=$(I);\
		[ -z "$$I" ] && I=download-$(C)-$(M);\
	avrdude \
		-p $(C) \
		$(call key,METHODS,$(M)) \
		-U eeprom:r:$$I-eeprom.hex:i \
		-U flash:r:$$I.hex:i \
		-U lock:r:-:h -U lfuse:r:-:h -U hfuse:r:-:h

uctest: C := m328p
uctest: M := arduinoisp__
uctest:
	avrdude \
		-p $(C) \
		$(call key,METHODS,$(M)) 

#flash: C := m8
#flash: M := usbasp
#flash: I := 
## Low-, high- and extended fuse
#flash: LF := 
#flash: HF := 
#flash: EF := 
## Lock/unlock bits
#flash: LB := 
#flash: UB := 
## Debug
#flash: D := echo
flash:
	@\
	( [ -n "$(HF)" ] && [ -n "$(LF)" ] || { exit 2; } ) && ( \
		[ -n "$(EF)" ] && { \
			$(D) sudo avrdude \
				-p $(C) -e \
				$(call key,METHODS,$(M)) \
				-U lock:w:$(UB):m \
				-U hfuse:w:$(HF):m \
				-U lfuse:w:$(LF):m \
				-U efuse:w:$(EF):m \
				;\
		} || { \
			$(D) sudo avrdude -p $(C) -e \
				$(call key,METHODS,$(M)) \
				-U lock:w:$(UB):m \
				-U hfuse:w:$(HF):m \
				-U lfuse:w:$(LF):m \
				;\
		};\
		[ -n "$(I)" ] && { \
			$(D) sudo avrdude -p $(C) -D \
				$(call key,METHODS,$(M)) \
				-U flash:w:$(I) \
				;\
		};\
		echo $(D) sudo avrdude \
			-p $(C) -e \
			$(call key,METHODS,$(M)) \
			-U lock:w:$(LB):m \
			;\
	)

### Preset common flash 

fusebit-doctor-m328: C := m328p
fusebit-doctor-m328: M := usbasp
fusebit-doctor-m328: I := firmware/atmega_fusebit_doctor_2.09_m328p.hex
#fusebit-doctor-m328: LF := 0x62
#fusebit-doctor-m328: HF := 0xD9
fusebit-doctor-m328: LF := 0x62
fusebit-doctor-m328: HF := 0xD1
fusebit-doctor-m328: EF := 0x07
fusebit-doctor-m328: LB := 0x3F
fusebit-doctor-m328: UB := 0x3F
fusebit-doctor-m328: flash
#fusebit-doctor-m328: D := 


fusebit-doctor-m8: C := m8
fusebit-doctor-m8: M := usbasp
fusebit-doctor-m8: I := atmega_fusebit_doctor_2.09_m8.hex
fusebit-doctor-m8: LF := 0xE1
fusebit-doctor-m8: HF := 0xD1
fusebit-doctor-m8: EF :=
fusebit-doctor-m8: LB := 0x3C
fusebit-doctor-m8: UB := 0x3F
fusebit-doctor-m8: D := 
fusebit-doctor-m8: flash


flash-betemcu: C := m8
flash-betemcu: M := usbasp
flash-betemcu: I := firmware/betemcu-usbasp/usbasp.2011-05-28/bin/firmware/usbasp.atmega8.2011-05-28.hex
flash-betemcu: LF := 0xFF
flash-betemcu: HF := 0xD9
flash-betemcu: EF := 
flash-betemcu: LB := 0x3C
flash-betemcu: UB := 0x3F
flash-betemcu: flash

upload-betemcu-usbasploader: M := usbasp
upload-betemcu-usbasploader: I := firmware/betemcu-usbasp/alternate_USBaspLoader_betemcu_timeout.hex
upload-betemcu-usbasploader:
	avrdude \
		-p m8 -e \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x3F:m
	avrdude \
		-p m8 \
		$(call key,METHODS,$(M)) \
		-U hfuse:w:0xC8:m -U lfuse:w:0xBF:m
	avrdude \
		-p m8 \
		$(call key,METHODS,$(M)) \
		-U flash:w:$(I)
	avrdude \
		-p m8 \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x0F:m

# from /home/berend/Application/arduino-0021/hardware/arduino/boards.txt
#m8-16Mhz: M := usbasp
m8-16Mhz: M := arduinoisp
#m8-16Mhz: I := firmware/ATmegaBOOT.hex
# from 1.0.3
m8-16Mhz: I := firmware/ATmegaBOOT-prod-firmware-2009-11-07.hex
m8-16Mhz:
	avrdude \
		-p m8 -e \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x3F:m -U lfuse:w:0xDF:m -U hfuse:w:0xCA:m
	avrdude \
		-p m8 \
		$(call key,METHODS,$(M)) \
		-D -U flash:w:$(I)
	avrdude \
		-p m8 \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x0F:m

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

m32-DI-8Mhz-int: M := arduinoisp
m32-DI-8Mhz-int: I := firmware/ATmegaBOOT_168_diecimila.hex
m32-DI-8Mhz-int:
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

# 16Mhz ext, 5V, delay bootloader
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

# Internal 8Mhz/57600baud 328 target
m328p-8Mhz: I := firmware/ATmegaBOOT_168_atmega328_pro_8MHz.hex
m328p-8Mhz: M := arduinoisp
m328p-8Mhz:
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

m328p-16Mhz: I := firmware/ATmegaBOOT_168_atmega328.hex
#m328p-16Mhz: I := firmware/optiboot_atmega328.hex
m328p-16Mhz: M := usbasp
m328p-16Mhz:
	avrdude \
		-p m328p -e \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x3F:m -U lfuse:w:0xFF:m -U hfuse:w:0xDA:m -U efuse:w:0x07:m
	avrdude \
		-p m328p \
		$(call key,METHODS,$(M)) \
		-D -U flash:w:$(I)
	avrdude \
		-p m328p \
		$(call key,METHODS,$(M)) \
		-U lock:w:0x0F:m

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

cassette328p: D := $/
cassette328p: 
	cd $D;\
	make arduino upload P=Mpe/Cassette328P I=Mpe/Cassette328P/Cassette328P.hex


m1284p: M := arduinoisp
m1284p: 
	avrdude \
		-p m1284p \
		$(call key,METHODS,$(M)) 

#      ------------ -- 
# 
# Integrating with another makefile for easy builds

ARDUINODIR := /home/berend/Application/arduino-1.0.3

# Build anything in target folder 'P'
#arduino: P :=
#arduino: B := 
arduino: LIB := $/libraries/
arduino: TARGETS := clean all
arduino: 
	p=$$(realpath .);\
	cd $P; \
		ARDUINODIR=$(ARDUINODIR) \
		BOARD=$(B) \
		make -f $$p/arduino.mk $(TARGETS)

#jeenode: C := m328p
jeenode: B := atmega328
jeenode: M := arduino
jeenode: arduino

isp_flash: P := $/libraries/jeelib/examples/Ports/isp_flash/
isp_flash: B := atmega328
isp_flash: arduino

arduino-boards:
	@p=$$(realpath .);\
	echo ARDUINODIR=$(ARDUINODIR);\
	make -f $$p/arduino.mk boards

### Common preset uploads

jeenodeisp: I := firmware/isp_flash_m328p.hex
jeenodeisp: X := -D
jeenodeisp: jeenode

#jeenode-isp-repair: P = $/libraries/jeelib/examples/Ports/isp_repair2/
#jeenode-isp-repair: arduino
#
#jeenode-isp-repair-post: DIR := $/
#jeenode-isp-repair-post:
#	cp $(DIR)libraries/jeelib/examples/Ports/isp_repair2/isp_repair2.hex firmware/isp_repair_m328p.hex

jeenodeisp-repair: C := m328p
jeenodeisp-repair: M := arduino
jeenodeisp-repair: I := firmware/isp_repair2_m328p.hex
#jeenodeisp-repair: X := -D
jeenodeisp-repair: upload

blinkall: C := m328p
blinkall: P := Mpe/BlinkAll
blinkall: I := Mpe/BlinkAll/BlinkAll.hex
blinkall: jeenode upload

3way: C := m328p
3way: P := Mpe/eBay-ThreeWayMeter/
3way: I := Mpe/eBay-ThreeWayMeter/Prototype.hex
3way: jeenode upload

radiolink: C := m328p
radiolink: P := Mpe/RadioLink/
radiolink: I := Mpe/RadioLink/RadioLink.hex
radiolink: jeenode upload

carriercase: C := m328p
carriercase: P := Mpe/CarrierCase/
carriercase: I := Mpe/CarrierCase/CarrierCase.hex
carriercase: jeenode upload

#      ------------ -- 
#
include                $(MK_SHARE)Core/Main.dirstack-pop.mk
# vim:noet:
