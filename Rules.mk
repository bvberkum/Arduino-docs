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
		arduino="-c arduino -P /dev/ttyUSB0 -b 57600" \
		arduinoisp="-cstk500v1 -P/dev/ttyUSB0 -b19200" \
	  usbasp="-c usbasp"
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

#      ------------ -- 
#
include                $(MK_SHARE)Core/Main.dirstack-pop.mk
# vim:noet:
