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

METHODS := \
		arduino="-c arduino -P /dev/ttyUSB0 -b 57600"; \
		arduinoisp="-cstk500v1 -P/dev/ttyUSB0 -b19200"; \
	  usbasp="-c usbasp -P usb"
#IMAGES := \
#	blink=firmware/betemcu-usbasp/misc/betemcu_blink/betemcu_blink.cpp.hex\
#	ArduinoISP=ArduinoISP_mega328.hex
#atmega8_mkjdz.com_I2C_lcd1602.hex


upload: C := m328p
upload: M := arduino
upload: I := firmware/ArduinoISP_mega328.hex
upload: X := -D
upload:
	avrdude \
		-v \
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
	sudo avrdude -v -v -v -p $(C) \
		$(call key,METHODS,$(M)) \
		-U eeprom:r:$$I-eeprom.hex:i \
		-U flash:r:$$I.hex:i \
		-U lock:r:-:h -U lfuse:r:-:h -U hfuse:r:-:h

upload-betemcu: M := usbasp
upload-betemcu:
	sudo avrdude -p m8 -e \
		$(call key,METHODS,$(M)) \
  	-U lock:w:0x3F:m -U hfuse:w:0xD9:m -U lfuse:w:0xFF:m
	sudo avrdude -p m8 -v -D \
		$(call key,METHODS,$(M)) \
  	-U flash:w:firmware/betemcu-usbasp/usbasp.2011-05-28/bin/firmware/usbasp.atmega8.2011-05-28.hex
	sudo avrdude -p m8 \
		$(call key,METHODS,$(M)) \
  	-U lock:w:0x3C:m

# Cannot re-read protected flash without -e?
#verify-betemcu: M := usbasp
#verify-betemcu:
#	sudo avrdude -p m8 \
#		$(call key,METHODS,$(M)) \
#  	-U lock:w:0x3F:m 
#	sudo avrdude -p m8 -v -v \
#		$(call key,METHODS,$(M)) \
#		-U flash:v:firmware/betemcu-usbasp/usbasp.2011-05-28/bin/firmware/usbasp.atmega8.2011-05-28.hex:i
#		-U lock:v:0x3C:m -U lfuse:v:0xff:m -U hfuse:v:0xd9:m
#	sudo avrdude -p m8 \
#		$(call key,METHODS,$(M)) \
#  	-U lock:w:0x3C:m 

#      ------------ -- 
#
include                $(MK_SHARE)Core/Main.dirstack-pop.mk
# vim:noet:
