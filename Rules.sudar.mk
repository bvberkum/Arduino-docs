### For detailed explanations about all of the available options, please refer
### to https://github.com/sudar/Arduino-Makefile/blob/master/arduino-mk-vars.md
### Original project where this Makefile comes from: https://github.com/WeAreLeka/Bare-Arduino-Project

ARDMK_DIR         = /usr/local/opt/arduino-mk
#ARDMK_DIR         = /usr/local/Homebrew/Library/Taps/sudar/homebrew-arduino-mk
#ARDMK_DIR         = /usr/local/var/homebrew/linked/arduino-mk

ARDUINO_DIR       = /Applications/Arduino.app/Contents/Java
#ARDUINO_DIR       = /usr/share/arduino

USER_LIB_PATH    :=  $(wildcard ~/project/Arduino-docs)/libraries

MONITOR_BAUDRATE  = 57600

AVR_TOOLS_DIR     = /usr/local
#AVR_TOOLS_DIR     = /usr

AVRDUDE           = /usr/local/bin/avrdude
#AVRDUDE           = /usr/bin/avrdude

CFLAGS_STD        = -std=gnu11

CXXFLAGS_STD      = -std=gnu++11

CXXFLAGS         += -pedantic -Wall -Wextra

MONITOR_PORT      = /dev/tty.usbmodem*


#PROJECT_DIR       = $(wildcard ~/project/Arduino-docs/)
#PROJECT_DIR       = $(wildcard ~/project/Arduino-docs/Prototype/SensorNode)
#BOARD_TAG         = uno
#CURRENT_DIR       = $(shell basename $(CURDIR))
#OBJDIR            = $(PROJECT_DIR)/bin/$(BOARD_TAG)/$(CURRENT_DIR)


#      ------------ -- 

# Build anything in target folder 'P'
#arduino: P :=
#arduino: BRD := 
#_arduino: LIB := $/libraries/
#_arduino: TARGETS := clean all
_arduino:
	@\
	$(call log,header2,BRD:sub,$(BRD):$(BOARD_SUB));\
	$(call log,header2,P,$(P));\
	p=$$(realpath $$(pwd));\
	test -n "$(P)" || { echo "Path expected"; exit 1; };\
	cd $(P) && \
		BN="$$(basename "$(P)" .hex)"; \
		make -f $(ARDMK_DIR)/Arduino.mk \
			CURRENT_DIR=$(shell basename $(CURDIR)) \
			PROJECT_DIR=$(P) \
			BOARD_TAG=$(BRD) \
			BOARD_SUB=$(BOARD_SUB) \
			OBJDIR=".";


#      ------------ -- 

arduino: TARGETS := target
#arduino: BRD := atmega328
arduino: _arduino

#jeenode: BRD := uno
jeenode: BRD := pro
jeenode: BOARD_SUB := 16MHzatmega328
#jeenode: BRD := 8MHzatmega328
jeenode: M := arduino
jeenode: arduino

#      ------------ -- 

upload: C := m328p
upload: M := arduino
upload: X := -D
upload: _upload

#      ------------ -- 
