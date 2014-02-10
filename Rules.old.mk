# Arduino-mpe/Rules.mk
MK_$d               := $/Rules.old.mk
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

ifeq ($(shell uname),"Linux")
PORT := /dev/tty.usbserial-A900TTH0
else
PORT := /dev/ttyUSB0
endif

listen: D := $(PORT)
listen: B := 57600
#listen: B := 38400
listen:
	@\
	$(ll) attention $@ "Starting minicom @$(B) baud.." $(D);\
	minicom -D $(D) -b $(B) minirc.arduino
	@\
	$(ll) ok $@ "minicom ended." $(D)

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

_upload:
	@\
	$(ll) attention $@ "Starting flash upload to $(C) using $(M).." $(I);\
	[ "$(M)" = "usbasp" ] && { sudo="sudo "; } || { sudo=; }; \
	$(D) $${sudo}avrdude \
		$(call key,METHODS,$(M))\
		-p $(C) \
		-U flash:w:$(I) \
		$(X) ; \
	$(ll) OK $@ "Flash upload completed successfully"

download: C := m328p
download: M := arduino
download: I := 
download: X := -D
download:
	@\
	$(ll) attention $@ "Starting flash/eeprom download using $(M).." $(I);\
	I=$(I);\
		[ -z "$$I" ] && I=download-$(C)-$(M);\
	avrdude \
		-p $(C) \
		$(call key,METHODS,$(M)) \
		-U eeprom:r:$$I-eeprom.hex:i \
		-U flash:r:$$I.hex:i \
		-U lock:r:-:h -U lfuse:r:-:h -U hfuse:r:-:h
	$(ll) OK $@ "Download completed successfully" $$I-*

_uctest:
	@\
	$(ll) attention $@ "Testing for $(C) using $(M).." $(PORT);\
	[ "$(M)" = "usbasp" ] && { sudo="sudo "; } || { sudo=; }; \
	$(D) $${sudo}avrdude \
		-p $(C) \
		$(call key,METHODS,$(M)) \
		$(X)
	@$(ll) OK $@

uctest: C := m328p
uctest: M := arduinoisp
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
	avrdude \
		-p $(C) $(call key,METHODS,$(M)) -D;\
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
	X="$(X)";\
	( [ -n "$(HF)" ] && [ -n "$(LF)" ] || { exit 1; } ) && ( \
		[ -n "$(UB)" ] && { \
			$(ll) attention $@ "Unlocking & erasing.." && \
			$(D) $${sudo}avrdude \
				-p $(C) -e $$X \
				$(call key,METHODS,$(M)) \
				-U lock:w:$(UB):m \
				|| exit 2;\
		}; \
		$(ll) attention $@ "Writing new fuses.." && \
		([ -n "$(EF)" ] && { \
			$(D) $${sudo}avrdude \
				-p $(C) -e $$X \
				$(call key,METHODS,$(M)) \
				-U hfuse:w:$(HF):m \
				-U lfuse:w:$(LF):m \
				-U efuse:w:$(EF):m \
				&& $(ll) info $@ OK \
				|| exit 3;\
		} || { \
			$(D) $${sudo}avrdude -p $(C) -e $$X \
				$(call key,METHODS,$(M)) \
				-U hfuse:w:$(HF):m \
				-U lfuse:w:$(LF):m \
				&& $(ll) info $@ OK \
				|| exit 4;\
		});\
		([ -n "$(E)" ] && { \
			$(ll) attention $@ "Writing EEPROM.." && \
			$(D) $${sudo}avrdude -p $(C) -D $$X \
				$(call key,METHODS,$(M)) \
				-U eeprom:w:$(E) \
				&& $(ll) info $@ OK \
				|| exit 5;\
		});\
		([ -n "$(I)" ] && { \
			$(ll) attention $@ "Writing Flash.." && \
			$(D) $${sudo}avrdude -p $(C) -D $$X \
				$(call key,METHODS,$(M)) \
				-U flash:w:$(I) \
				&& $(ll) info $@ OK \
				|| exit 6;\
		});\
		([ -n "$(LB)" ] && { \
			$(ll) attention $@ "Locking.." && \
				$(D) $${sudo}avrdude $$X \
					-p $(C) -D \
					$(call key,METHODS,$(M)) \
					-U lock:w:$(LB):m \
					&& $(ll) info $@ OK \
					|| exit 7;\
		}); \
	)

### Preset common flash 

fusebit-doctor-m328: C := m328p
fusebit-doctor-m328: M := usbasp
fusebit-doctor-m328: I := firmware/atmega_fusebit_doctor_2.11_m328p.hex
#fusebit-doctor-m328: LF := 0x62
#fusebit-doctor-m328: HF := 0xD9
fusebit-doctor-m328: LF := 0x62
fusebit-doctor-m328: HF := 0xD2
fusebit-doctor-m328: EF := 0x07
fusebit-doctor-m328: LB := 0x0F
fusebit-doctor-m328: UB := 0x3F
fusebit-doctor-m328: X := -B 3
fusebit-doctor-m328: _flash
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
fusebit-doctor-m8: _flash


flash-betemcu: C := m8
flash-betemcu: M := usbasp
flash-betemcu: I := firmware/betemcu-usbasp/usbasp.2011-05-28/bin/firmware/usbasp.atmega8.2011-05-28.hex
flash-betemcu: LF := 0xFF
flash-betemcu: HF := 0xD9
flash-betemcu: EF := 
flash-betemcu: LB := 0x3C
flash-betemcu: UB := 0x3F
flash-betemcu: _flash

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
# This is working with 19200 baud
m8-16Mhz: C := m8
m8-16Mhz: M := usbasp
#m8-16Mhz: I := firmware/ATmegaBOOT.hex
# from 1.0.3
m8-16Mhz: I := firmware/ATmegaBOOT-prod-firmware-2009-11-07.hex
m8-16Mhz: HF := 0xCA
m8-16Mhz: LF := 0xDF
m8-16Mhz: LB := 0x0F
m8-16Mhz: UB := 0x3F
m8-16Mhz: X := -B 3 
m8-16Mhz: _flash

# not sure what this is
m8-fd: C := m8
m8-fd: M := usbasp
m8-fd: HF := 0x99
m8-fd: LF := 0XC1
m8-fd: LB := 0x0F
m8-fd: UB := 0x3F
m8-fd: X := -B 3 
m8-fd: _flash

# 8Mhz optiboot, cannot get this to work at 9600b
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

# 8Mhz noxtal
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

#m328p-16Mhz: I := firmware/optiboot_atmega328.hex

m328p-16Mhz: I := firmware/ATmegaBOOT_168_atmega328.hex
m328p-16Mhz: M := usbasp
m328p-16Mhz: X :=
m328p-16Mhz: C := m328p
m328p-16Mhz: LF := 0xFF
m328p-16Mhz: HF := 0xDA
m328p-16Mhz: EF := 0x07
m328p-16Mhz: LB := 0x0F
m328p-16Mhz: UB := 0x3F
m328p-16Mhz: _flash

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

ARDUINODIR := /home/berend/Application/arduino-1.0.3
#ARDUINODIR := /usr/share/arduino/
#ARDUINODIR := /home/berend/Application/arduino-1.0.3
#ARDUINODIR := /usr/share/arduino/
#ARDUINODIR := $(shell realpath ./arduino-1.0.5)
#ARDUINODIR := $(shell realpath ./arduino-1.0.1)
ARDUINODIR := $(shell realpath ./arduinodir)

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
	p=$$(realpath .);\
	cd $P; \
		ARDUINODIR=$(ARDUINODIR) \
		AVRTOOLSPATH="$(AVRTOOLSPATH)" \
		BOARD=$(BRD) \
		make -f $$p/arduino.mk $(TARGETS)

arduino: TARGETS := clean all
arduino: _arduino

jeenode: BRD := atmega328
jeenode: M := arduino
jeenode: arduino

arduino-firmware: BRD := 
arduino-firmware: P := 
arduino-firmware: TARGETS := 
arduino-firmware: _arduino-firmware

_arduino-firmware:
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

isp_flash: P := $/libraries/jeelib/examples/Ports/isp_flash/
isp_flash: I := $/libraries/jeelib/examples/Ports/isp_flash/isp_flash.hex
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

blink: C := m328p
blink: P := Mpe/Blink
blink: I := Mpe/Blink/Blink.hex
blink: jeenode _upload

blinkall: C := m328p
blinkall: P := Mpe/BlinkAll
blinkall: I := Mpe/BlinkAll/BlinkAll.hex
blinkall: jeenode upload

3way: C := m328p
3way: P := Mpe/eBay-ThreeWayMeter/
3way: I := Mpe/eBay-ThreeWayMeter/Prototype.hex
3way: jeenode upload

rf12demo: C := m328p
rf12demo: P := libraries/jeelib/examples/RF12/RF12demo/
rf12demo: I := libraries/jeelib/examples/RF12/RF12demo/RF12demo.hex
rf12demo: jeenode upload

radioblip: C := m328p
radioblip: P := Mpe/RadioBlip/
radioblip: I := Mpe/RadioBlip/RadioBlip.hex
radioblip: jeenode upload

jeeblip: C := m328p
jeeblip: P := libraries/jeelib/examples/RF12/radioBlip/
jeeblip: I := libraries/jeelib/examples/RF12/radioBlip/radioBlip.hex
jeeblip: jeenode upload

radiolink: C := m328p
radiolink: P := Mpe/RadioLink/
radiolink: I := Mpe/RadioLink/RadioLink.hex
radiolink: jeenode upload

carriercase: BRD := uno
carriercase: P := Mpe/CarrierCase/
carriercase: _arduino-firmware upload
#jeenode upload

cassette328p: C := m328p
cassette328p: P := Mpe/Cassette328P/
cassette328p: I := Mpe/Cassette328P/Cassette328P.hex
cassette328p: jeenode upload

hanrun: C := m328p
hanrun: P := Misc/HanrunENC28J60/
hanrun: I := Misc/HanrunENC28J60/HanrunENC28J60.hex
hanrun: jeenode upload

fuseboxmon: C := m328p
fuseboxmon: P := libraries/jeelib/examples/RF12/p1scanner
fuseboxmon: I := libraries/jeelib/examples/RF12/p1scanner/p1scanner.hex
fuseboxmon: jeenode upload

utilitybug: C := m328p
utilitybug: P := Mpe/Utility/UtilityBug/
utilitybug: I := Mpe/Utility/UtilityBug/UtilityBug.hex
utilitybug: jeenode upload

gasdetector: C := m328p
gasdetector: P := Mpe/GasDetector/
gasdetector: I := Mpe/GasDetector/GasDetector.hex
gasdetector: jeenode upload

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

avrtransistortester: E := Misc/AVR-Transistortester_neu/ATmega8/TransistorTestNew.eep
avrtransistortester: I := Misc/AVR-Transistortester_neu/ATmega8/TransistorTestNew.hex
# XXX: This one gives timeout, circuit change?
#avrtransistortester: E := Mpe/transistortester/Software/trunk/mega8/TransistorTester.eep
#avrtransistortester: I := Mpe/transistortester/Software/trunk/mega8/TransistorTester.hex
avrtransistortester: M := usbasp
avrtransistortester: LF := 0xC1
avrtransistortester: HF := 0xD9
avrtransistortester: UB := 0x3F
avrtransistortester: LB := 0x30
avrtransistortester: C := m8
avrtransistortester: X := -B 3
avrtransistortester: _flash

at85blink: M:=usbasp
at85blink: BRD:=t85
at85blink: C:=t85
at85blink: X:=-B 3
at85blink: P:=Mpe/Blink
at85blink: I:=Mpe/Blink/Blink.hex
at85blink: TARGETS:= clean all
at85blink: _arduino _upload

at85usb: D:=sudo
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
#int 1Mhz:
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

all_i2c: C := m328p
all_i2c: BRD := atmega328
all_i2c: P := Mpe/i2c_all/
all_i2c: I := Mpe/i2c_all/i2c_all.hex
all_i2c: _arduino upload

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

soarer: I := Misc/Soarer_Convertor/tmp/Soarer_at2usb_v1.12_atmega32u4.hex
soarer: M := leonardo
soarer: C := atmega32u4 
soarer: PORT := /dev/ttyACM0
soarer: X := -D -v
soarer: _upload

jeedht: C := m328p
jeedht: P := Mpe/DHT11Test/JeeLibDHT/
jeedht: I := Mpe/DHT11Test/JeeLibDHT/JeeLibDHT.hex
jeedht: jeenode upload

adadht: C := m328p
adadht: P := Mpe/DHT11Test/AdafruitDHT/
adadht: I := Mpe/DHT11Test/AdafruitDHT/AdafruitDHT.hex
adadht: jeenode upload

# Leonardo mega32u4 / teensy 2.0?


library: $(realpath $/libraries/)
	cd arduinodir/libraries/ \
		&& ~/bin/init-symlinks-relative.sh $<

#      ------------ -- 
#
# vim:noet:

