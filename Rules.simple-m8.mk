# Testing simpler build without Arduino overhead
#
## http://www.nongnu.org/avr-libc/user-manual/group__demo__project.html
simple-m8:
	cd libraries/avr_lib_ds1307_01/src/;\
	avr-gcc -g -Os -mmcu=atmega8 -c main.c
	avr-gcc -g -mmcu=atmega8 -o main.elf main.o
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
