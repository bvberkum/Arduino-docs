$(module-header,new,$/Rules.new.mk)
#
#      ------------ --

ARDUINO_DIR := /Applications/Arduino.app
BOARDS_TXT := $(ARDUINO_DIR)/Contents/Java/hardware/arduino/avr/boards.txt
ARDUINO_SKETCHBOOK := ~/project/arduino-docs

#ARDMK_DIR     = /usr/local
#AVR_TOOLS_DIR = /usr
#MONITOR_PORT  = /dev/ttyACM0

ALTERNATE_CORE := $(AC)
BOARD_TAG := $(BRD)

INO_MK := /usr/local/opt/arduino-mk/Arduino.mk



copy-from-ino15:
	mkdir -vp hardware/

	rsync -avzui \
		~/Library/Arduino15/packages/ hardware/adafruit


arduino-mk := make \
		ARDUINO_SKETCHBOOK=$(ARDUINO_SKETCHBOOK) \
		MAKE="make -f $(INO_MK)" \
		-f $(INO_MK)

ifneq ($(ALTERNATE_CORE),)
arduino-mk := $(arduino-mk) \
		ALTERNATE_CORE=$(ALTERNATE_CORE)
endif

ifneq ($(DEFINES),)
arduino-mk := $(arduino-mk) \
		DEFINES="$(DEFINES)"
endif

ifneq ($(BOARD_TAG),)
arduino-mk-run := $(arduino-mk) \
		BOARD_TAG=$(BOARD_TAG)
else
arduino-mk-run := $(arduino-mk) \
		BOARD_TAG=atmega328
endif



help::
	@touch arduino-docs.ino
	@$(arduino-mk) $@
	@touch arduino-docs.ino

clean::
	@touch arduino-docs.ino
	@$(arduino-mk) $@
	@touch arduino-docs.ino

boards:
	@$(ll) attention $@ "Listing boards"
	@#grep \.name= $(BOARDS_TXT) | cut -f 1 -d . | column
	@touch arduino-docs.ino
	@$(arduino-mk) show_boards
	@echo "Var aliases:"
	@echo "  AC -> ALTERNATE_CORE"
	@echo "  BRD -> BOARD_TAG"
	@touch arduino-docs.ino


blink:
	@cd Prototype/Blink; $(arduino-mk-run) ; $(arduino-mk-run) upload

tinyrf24:
	@cd Mpe/TinyRF24/; $(arduino-mk-run) ; $(arduino-mk-run) upload



