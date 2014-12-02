# Testing simpler build without Arduino overhead
#
## http://www.nongnu.org/avr-libc/user-manual/group__demo__project.html

BOARD:=atmega8

# default path for avr tools
ifndef AVRTOOLSPATH
AVRTOOLSPATH := $(subst :, , $(PATH))
AVRTOOLSPATH += $(ARDUINODIR)/hardware/tools
AVRTOOLSPATH += $(ARDUINODIR)/hardware/tools/avr/bin
endif

# software
findsoftware = $(firstword $(wildcard $(addsuffix /$(1), $(AVRTOOLSPATH))))
CC := $(call findsoftware,avr-gcc)
CXX := $(call findsoftware,avr-g++)
LD := $(call findsoftware,avr-ld)
AR := $(call findsoftware,avr-ar)
OBJCOPY := $(call findsoftware,avr-objcopy)
AVRDUDE := $(call findsoftware,avrdude)
AVRSIZE := $(call findsoftware,avr-size)

TARGET := libraries/avr_lib_ds1307_01/src/main

OBJECTS := $(addsuffix .o, $(basename $(SOURCES)))
DEPFILES := $(patsubst %, .dep/%.dep, $(SOURCES))
SOURCES := $(TARGET).c 

# obtain board parameters from the arduino boards.txt file
BOARDS_FILE := $(ARDUINODIR)/hardware/arduino/boards.txt
BOARD_BUILD_MCU := \
	$(shell sed -ne "s/$(BOARD).build.mcu=\(.*\)/\1/p" $(BOARDS_FILE))
BOARD_BUILD_FCPU := \
	$(shell sed -ne "s/$(BOARD).build.f_cpu=\(.*\)/\1/p" $(BOARDS_FILE))
BOARD_BUILD_VARIANT := \
	$(shell sed -ne "s/$(BOARD).build.variant=\(.*\)/\1/p" $(BOARDS_FILE))
BOARD_UPLOAD_SPEED := \
	$(shell sed -ne "s/$(BOARD).upload.speed=\(.*\)/\1/p" $(BOARDS_FILE))
BOARD_UPLOAD_PROTOCOL := \
	$(shell sed -ne "s/$(BOARD).upload.protocol=\(.*\)/\1/p" $(BOARDS_FILE))
BOARD_USB_VID := \
	$(shell sed -ne "s/$(BOARD).build.vid=\(.*\)/\1/p" $(BOARDS_FILE))
BOARD_USB_PID := \
	$(shell sed -ne "s/$(BOARD).build.pid=\(.*\)/\1/p" $(BOARDS_FILE))

CPPFLAGS += -mmcu=$(BOARD_BUILD_MCU)
CPPFLAGS += -DF_CPU=$(BOARD_BUILD_FCPU)
CPPFLAGS += -DUSB_VID=$(BOARD_USB_VID) -DUSB_PID=$(BOARD_USB_PID)
LINKFLAGS := -Os -Wl,--gc-sections -mmcu=$(BOARD_BUILD_MCU)



$(TARGET).hex: $(TARGET).elf
	@$(OBJCOPY) -O ihex -R .eeprom $< $@
	@echo -n .

$(TARGET).elf: $(OBJECTS)
	@$(CC) $(LINKFLAGS) $(OBJECTS) -lm -o $@
	@echo -n .

%.o: %.c
	@mkdir -p .dep/$(dir $<)
	$(COMPILE.c) $(CPPDEPFLAGS) -o $@ $<
	@echo -n .

xxx: $(TARGET).hex

simple:
	cd libraries/avr_lib_ds1307_01/src/;\
	echo "Compiling";\
	avr-gcc -g -Os -mmcu=atmega8 \
		-DF_CPU=16000000L\
		-c main.c \
		-I./uart/ -I./ds1307/ -I./ds1307/ -I. ;\
	echo "Linking";\
	avr-gcc -g -mmcu=atmega8 \
		-DF_CPU=16000000L\
		-I./uart/ -I./ds1307/ -I./ds1307/ -I. \
		-o main.elf main.o; \
	echo "Image copy";\
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex; 



