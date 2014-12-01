MCU = atmega328p
#MCU = atmega8
SRC = $(TARGET).c ../../uart/uart.c nrf24l01/nrf24l01.c spi/spi.c
CDEFS = -DF_CPU=16000000L -DUART_BAUD_RATE=57600
CINCS = -I. -I../../ 

