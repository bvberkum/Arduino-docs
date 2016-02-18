$(module-header,new,$/Rules.new.mk)
#
#      ------------ --

ARDUINO_DIR := /Applications/Arduino.app
BOARDS_TXT := $(ARDUINO_DIR)/Contents/Java/hardware/arduino/avr/boards.txt
ARDUINO_SKETCHBOOK := ~/project/arduino-docs

#ARDMK_DIR     = /usr/local
#AVR_TOOLS_DIR = /usr
#MONITOR_PORT  = /dev/ttyACM0

ifneq ($(C),arduino)
ALTERNATE_CORE := $(C)
endif
BOARD_TAG := $(BRD)

INO_MK := /usr/local/opt/arduino-mk/Arduino.mk



link-hardware-to-ino15:
	@mkdir -vp hardware/adafruit
	@cd hardware/adafruit; \
		test -h avr && rm avr; \
		ln -s ~/Library/Arduino15/packages/adafruit/hardware/avr/1.4.2 avr
	@mkdir -vp hardware/digistump
	@cd hardware/digistump/; \
		test -h avr && rm avr; \
		ln -s ~/Library/Arduino15/packages/digistump/hardware/avr/1.6.5 avr
	#rsync -avzui \
	#	~/Library/Arduino15/packages/ hardware/adafruit


arduino-mk := make \
		ARDUINO_SKETCHBOOK=$(ARDUINO_SKETCHBOOK) \
		MAKE="make "\
			--debug \
			-f $(INO_MK) \
			ARDUINO_VERSION=164 \
			DEVICE_PATH=$(PORT)

ifneq ($(ALTERNATE_CORE),)
arduino-mk := $(arduino-mk) \
	ALTERNATE_CORE=$(ALTERNATE_CORE) \
	ARDUINO_VAR_PATH=/Users/berend/project/arduino-docs/hardware/$(ALTERNATE_CORE)/avr/variants
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
	@echo "  C -> arduino|ALTERNATE_CORE"
	@echo "  BRD -> BOARD_TAG"
	@touch arduino-docs.ino


TRGT += arduino-upload

arduino-upload:
	@$(ll) header1 PREFIX "$(INO_PREF)"
	@$(ll) header1 CORE "$(C)"
	@$(ll) header1 BOARD "$(BRD)"
	@$(ll) header1 DEFINES "$(DEFINES)"
	@cd $(INO_PREF); $(arduino-mk-run) ; $(arduino-mk-run) upload


