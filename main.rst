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
  
  avrdude -v -p m8 -cstk500v1 -P/dev/ttyUSB0 -b19200 -D 

Using ISP, set the fuses and bootloader. Then use the other protocols for
program uploads. The lock bit will protect the bootloader, and load only the
program over serial--the Arduino way.

Common options::
  
  avrdude 
    -C /home/berend/Application/arduino-1.0.1/hardware/tools/avrdude.conf 
    -v -v -v -v 
    -c arduino -P /dev/ttyUSB0 -b 57600
    -D
    -p atmega328p

Upload ArduinoISP using standard Arduino::

    -U flash:firmware/ArduinoISP_mega328.hex

Upload usbasp bootloader using JeeNode ISP::

    avrdude -v -p m8 -c usbasp 
      -U hfuse:w:0xC8:m -U lfuse:w:0xBF:m -U lock:w:0x0F:m
      -U flash:w:firmware/betemcu-usbasp/alternate_USBaspLoader_betemcu_timeout.hex

Using usbasp:

avrdude -C/home/berend/Application/arduino-1.0.1/hardware/tools/avrdude.conf -v
-v -v -v -patmega8 -carduino -P/dev/ttyUSB0 -b19200 -D
-Uflash:w:/tmp/build7947190257849781585.tmp/atmega8l_usb.cpp.hex:i 

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

    avrdude -v -p m8 -c usbasp -U hfuse:w:0xC8:m -U lfuse:w:0xBF:m 
      -U flash:w:firmware/betemcu-usbasp/alternate_USBaspLoader.2010-07-27_configured_for_betemcu/firmware/hexfiles/alternate_USBaspLoader_betemcu_timeout.hex 
      -U lock:w:0x0F:m

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
  - upon connecting the jumper for reprogramming, the device is no longer
    recognized as usbasp.  

  * Problem: different behaviours upon reflash. 
  * Using two new betemcu's, one soldered to be reprogrammed. 
    Verify using ``make verify-betemcu``, yields these fuses:

    :hfuse: 0xd9
    :lfuse: 0xff
    :lock: 0x3c

    The same fuse results for usbasp or arduinoisp.
    However the eeprom memory dump is different.
    This is the betemcu image: <file://./firmware/betemcu-usbasp/usbasp_atmega8l_eeprom-betemcu_download.hex>
    Appearantly not needed, so excluding.

  * Also writing these settings on a previous (already modded) betemcu the fuse
    bits won't "stick" ``make upload-betemcu``:

    - lfuse 0xff reads out as 0xbf
    - hfuse 0xd8 reads 0xc8
    - lock is okay (0x3c).

    The problem seems independent of programmer. Strangely though one
    stick reads lock 0x3f? 

    After a little investigating it turns out I might have to unlock and then
    lock before writing flash, as indicated by `project ouroboros post`_.

  * Using previous observation, updated ``make upload-betemcu``. Will now erase,
    and set lock bit to value given in ouroboros project for avrdude (0x3F). 
    Then a second run to flash and set fuses, and then lock the lock bit. 
    The first erase, and turning of erase on second flash-write may be 
    important, its left untested.

    :unlock: 0x3F
    :lfuse: 0xFF
    :hfuse: 0xD9
    :lock: 0x0F

    This now enables reflashing a betemcu USB stick to usbasp using both JeeNode
    isp_flash (Arduino ISP) and another betemcu usbasp.

    I am using the firmware given by the ouroboros downloads. It is frustrating
    but my own download looks like garbage. Maybe also something to do with the
    fuses. A bit of fiddling suggest then -e  flag together with the unlock
    is needed, and rereading/verifying the flash might be impossible.
    

.. _project ouroboros post: http://jethomson.wordpress.com/2011/08/18/project-ouroboros-reflashing-a-betemcu-usbasp-programmer/

Firmware
---------
mx-usbisp-v3.00
  Not working.

betemcu.cn USBasp MiniProg
  Not working.

betemcu-usbasp/alternate_USBaspLoader_betemcu_timeout.hex
  An usbasp bootloader suitable for Atmega8L USB devices.

atmega8_mkjdz.com_I2C_lcd1602.hex
  Program data to run I2C LCD demo on USBasp 

ArduinoISP_mega328.hex
  Arduino as ISP.

isp_flash_m328p.hex
  Run a JeeNode as Arduino ISP (with the flash board).


Protocols
----------
TODO: mkII, usbasp, stk500v1

Downloads
---------
firmware/betemcu-usbasp/usbprog.rar
  From.  


------

betemcu 1 flash attempt using JeeNode ISP::

  sudo avrdude -p m8 -cstk500v1 -P/dev/ttyUSB0 -b19200 -U lock:w:0x3f:m -U hfuse:w:0xC8:m -U lfuse:w:0xBF:m
  sudo avrdude -p m8 -cstk500v1 -P/dev/ttyUSB0 -b19200 -v -U flash:w:firmware/betemcu-usbasp/alternate_USBaspLoader_betemcu_timeout.hex
  sudo avrdude -p m8 -cstk500v1 -P/dev/ttyUSB0 -b19200 -U lock:w:0x0F:m


betemcu 1 flash attempt using betemcu usbasp::

  sudo avrdude -p m8 -c usbasp -e -U lock:w:0x3F:m -U hfuse:w:0xD9:m -U lfuse:w:0xFF:m
  sudo avrdude -p m8 -c usbasp -D -v -U flash:w:firmware/betemcu-usbasp/usbasp.2011-05-28/bin/firmware/usbasp.atmega8.2011-05-28.hex
  sudo avrdude -p m8 -c usbasp -U lock:w:0x3C:m

betemcu 1 flash attempt using JeeNode ISP::

  sudo avrdude -p m8 -cstk500v1 -P/dev/ttyUSB0 -b19200 -e -U lock:w:0x3F:m -U hfuse:w:0xD9:m -U lfuse:w:0xFF:m
  sudo avrdude -p m8 -cstk500v1 -P/dev/ttyUSB0 -b19200 -D -v -U flash:w:firmware/betemcu-usbasp/usbasp.2011-05-28/bin/firmware/usbasp.atmega8.2011-05-28.hex
  sudo avrdude -p m8 -cstk500v1 -P/dev/ttyUSB0 -b19200 -U lock:w:0x3C:m



