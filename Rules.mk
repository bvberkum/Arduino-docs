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
# CLN += $/tmpfile
# TEST += $/testtarget

METHODS = \
		arduino="-c arduino -P $(PORT) -b 57600"; \
		arduinoisp="-cstk500v1 -P $(PORT) -b19200"; \
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

upload: C := m328p
upload: M := arduino
upload: I := firmware/ArduinoISP_mega328.hex
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
	sudo avrdude \
		-p $(C) \
		$(call key,METHODS,$(M)) \
		-U eeprom:r:$$I-eeprom.hex:i \
		-U flash:r:$$I.hex:i \
		-U lock:r:-:h -U lfuse:r:-:h -U hfuse:r:-:h

upload-betemcu: M := usbasp
upload-betemcu:
	sudo avrdude -p m8 -e \
		$(call key,METHODS,$(M)) \
  	-U lock:w:0x3F:m -U hfuse:w:0xD9:m -U lfuse:w:0xFF:m
	sudo avrdude -p m8 \
		-D \
		$(call key,METHODS,$(M)) \
  	-U flash:w:firmware/betemcu-usbasp/usbasp.2011-05-28/bin/firmware/usbasp.atmega8.2011-05-28.hex
	sudo avrdude -p m8 \
		$(call key,METHODS,$(M)) \
  	-U lock:w:0x3C:m

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
m328p-16Mhz: M := arduinoisp
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

# Cannot re-read protected flash without -e?
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

ARDUINODIR := ~/Application/arduino-1.0.3

# Build anything in target folder 'P'
arduino: P := 
arduino: B := atmega328
arduino: 
	p=$$(realpath .);\
	cd $P; \
		ARDUINODIR=$(ARDUINODIR) \
		BOARD=$(B) \
		make -f $$p/arduino.mk clean all

#      ------------ -- 
#
include                $(MK_SHARE)Core/Main.dirstack-pop.mk
# vim:noet:
