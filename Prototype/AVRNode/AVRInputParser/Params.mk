CDEFS := -DF_CPU=16000000L -DUART_BAUD_RATE=57600
CINCS := -I../../../libraries
SRC ?= $(TARGET).c  \
	../../../libraries/mpelib/ser.c \
	../../../libraries/mpelib/sercmd.c \
	../../../libraries/mpelib/atmega.c \
	../../../libraries/mpelib/avr.c \
	../../../libraries/uart/uart.c

