SRC := $(TARGET).c  ./ds1307/ds1307.c  ./i2chw/twimaster.c ../../uart/uart.c
CDEFS := -DF_CPU=16000000L
CINCS := -I../../
