Arduino and AVR (avrdude) related.

- there is no toolchain than arduino, though make or equiv would be nice to
  have.
- this dir is used as main sketches folder for Arduino <~/Documents/Dev/Arduino>

* Normally AVRISP mkII uploads sketches to m328p over an USB BUB II module (w/ FTDI TF232RL chip).
* Alternatively, the JeeNode is used as ISP using JeeLab's Flash Board. This is
  compatible with 


Arduino/AVRdude
----------------
avrdude
  -U <memtype>:r|w|v:<filename>[:format]

Using Arduino as ISP::
  
  avrdude -v -p m8 -cstk500v1 -P/dev/ttyUSB0 -b19200 


Module support
--------------
USBisp ``mx-usbisp-v3.00``
  :Device signature: 0x1e9307
  :fuse bits:
    :hfuse: 0xbf
    :lfuse: 0xcf
    :lock: 0x3c

  I'm not sure if the delivered device is supposed to do anything, I cant test
  it outside of Linux, and I'm pretty sure it's not doing anything there.

  - an tiny Atmega8L USB package with colored slide on metal cover and AVR isp
    compatible header IDC header. Came with about 60cm flatcable. 
  - Blue and red onboard SMT LEDs, under a milimiter sized hole 
    drilled in the aluminium cover. At arduino pins 14 (blue) and 15 (red). 
  - Modded: added two buttons, one to reset, one to enable reprogramming the
    application (using USBaspLoader, to reflash bootloader another USBasp module is
    used). Attaching program switch does not look feasible at all, need need to use
    USBaspLoader bootloader image with timeout setting.
  - modded: removed surplus GND header pins (that would normally alternate the MOSI, 
    MISO, and SCK cores in a flat cable) and nc pin, with intention to route SDA/SCL
    and TX/RX, but chip is to small to solder. At least connector is compatible
    with other USBasp mods.

  * Cannot be modded further than adding reset. SPI pins available only, chip is
    too small.
  * Usable for arduino projects with SPI and USB toys.
  
  - Programmed using another USB module, an usbasp from betemcu::

    avrdude -v -p m8 -c usbasp -U hfuse:w:0xC8:m -U lfuse:w:0xBF:m -U flash:w:./firmware/betemcu-usbasp/alternate_USBaspLoader.2010-07-27_configured_for_betemcu/firmware/hexfiles/alternate_USBaspLoader_betemcu_timeout.hex -U lock:w:0x0F:m

  - Now it accepts any program using arduino protocol, e.g. 
   `vusb_mouse_example.hex` which turns the stick into a mouse device that
    slowly circles the cursor over your screen.
  - It can be turned into an usbasp programmer itself by uploading the original 
    firmware to flash again::

      avrdude -v -p m8 -c usbasp -U flash:w:mx-usbisp-v3.00-flash.hex 

    Just press the reset, note that blue led lights up and then start avrdude.

  More info with ouroboros project using USBaspLoader.

USBasp ```` MiniProg
  - from betemcu.cn, Atmega8L TQFP. Yellow led (D4) at m8 PC0: and red (D3) at PC1.
  - Moddable to route I2C/TWI (SDA/SCL) and serial (TX/RX). Additional routes
    with glued on female jumper strip (16 extra pins should be enough for
    almost all spare atmega pins).
  - no suitable project box or cover. 

  :defaults:
    :hfuse: 0xd9
    :lfuse: 0xff
    :lock: 0x3c

  ::
      avrdude -v -p m8 -c usbasp -U eeprom:r:betemcu-eeprom-firmware.hex:h -U flash:r:betemcu-flash-firmware.hex:h

  ::
      sudo avrdude -v -p m8 -cstk500v1 -P/dev/ttyUSB0 -b19200 
            -Ulfuse:w:0xff:m -Uhfuse:w:0xd9:m \
            -Ueeprom:w:firmware/betemcu-usbasp/eeprom-2012-11-18.hex:h \
            -Uflash:w:firmware/betemcu-usbasp/flash-2012-11-18.hex:h 

  http://jethomson.wordpress.com/2011/08/18/project-ouroboros-reflashing-a-betemcu-usbasp-programmer/

Firmware
---------
mx-usbisp-v3.00
  Not working.

betemcu.cn USBasp MiniProg
  Not working.

Protocols
----------
TODO: mkII, usbasp, stk500v1


