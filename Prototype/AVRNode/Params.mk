MCU := atmega328p
CDEFS := -DF_CPU=16000000L -DUART_BAUD_RATE=57600
CINCS := -I../../libraries
SRC ?= $(TARGET).c ../../libraries/uart/uart.c
