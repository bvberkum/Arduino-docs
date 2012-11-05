Arduino and AVR (avrdude) related.

Use as main sketches folder for Arduino <~/Documents/Dev/Arduino>


bootloader/
  mx-usbisp-v3.00
    Identified by board markings, the fuse settings and firmware of the Atmega8L chip.

    Device signature = 0x1e9307

    lfuse  0xbf
    hfuse  0xcf
    lock   0x3c
    flash `hex` `i`
    eepromp `hex` `i`


avrdude
  -U <memtype>:r|w|v:<filename>[:format]

