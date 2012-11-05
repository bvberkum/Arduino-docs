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

    I'm not sure if the delivered device is supposed to do anything, I cant test
    it outside of Linux, and I'm pretty sure it's not doing anything there.

avrdude
  -U <memtype>:r|w|v:<filename>[:format]


