Arduino and AVR (avrdude) projects
==================================
- The files in this repository are from my Arduino/sketchbook folder,
  which I keep at <~/Documents/Dev/Arduino>. 



Headless build
--------------
The sketches can be build without starting the Arduino frontend:

- Edam's arduino.mk makes building easy using the Arduino source package,
  it only needs a pointer to the current Arduino folder.
- mkdoc Rules.mk is used for the rest, uploading, downloading, flashing bits, building docs, etc.
- Example::

    $ make arduino P=Mpe/Blink B=atmega328p
    $ make upload I=Mpe/Blink/Blink.hex C=m328p M=arduino 

  Read the source for more on the parameters.
- I uses AVRISP mkII to upload sketches to m328p, using an USB BUB II module (w/ FTDI TF232RL chip).
- Alternatively, the JeeNode is used as ISP using JeeLab's Flash Board to upload
  bootloaders.

- **Libraries** are scanned for based on name, to inlude 'myLib.h' the file must
  be in 

* JeeNode Fuses stuff http://forum.jeelabs.net/node/848

Arduino/AVRdude
----------------
An introduction to the world of ~.
Lets introduce the main boards. 

Arduino NG (Nuova Generazione)
  - An USB board with ATmega8, later ATmega168.
Diecimilla "10.000"
  - Celebration of 10.000 boards. Has an ATmega168 on it. PDIP.
Duemilanove "2009"
  - First an ATmega168, then ATmega328p. PDIP.
UNO
  - USB board. Now at rev3. PDIP.
JeeNode
  - Six versions, all 3.3v.
    The mainline is compatible with Duemilanove, and later 
    with Uno (OptiBoot 0.44). [#]_

As usual, 8, 168 and 328 are interchangeable with notes on implementation
details and implication for applications.
The same seems to go for 16/32/644/1284, but I need to research that.

Now, to write software to the boards: avrdude. Involved hardware is discussed
later. The chips have fuses and two memory areas; eeprom and flash, the syntax to 
access::

  avrdude
    -U <memtype>:r|w|v:<filename>[:format]

After fuses and a bootloader has been "burned" we can use the Arduino way of
uploading "sketches": we hook a serial device which has DTR attached to the
reset line. After the reset the chip now accepts a new program which it writes
after the existing bootloader which is now protected memory area, I'm not sure 
on the protocol but that is how I understand it.

The initial burning knows several methods and phases. I've read new chips need
to be HV programmed once. Generally I have been successfull using arduinoisp 
and USBasp interfaces to burn the fuses and bootloader. I think both are 5V.
Once the protection bits are set in the fuses, a reburn requires the ``-e`` flag 
to erase the chip first. The makefile uses flag ``-D`` normally which prevents
erase.

Using Arduino as ISP (I use a JeeNode but avrdude works the same, although the 
actual on-chip software and pinout may be different)::
  
  avrdude -v -cstk500v1 -P/dev/ttyUSB0 -b 19200

Or using USBasp::

  avrdude -v -c usbasp -P usb

Common options::
  
  avrdude 
    -C /home/berend/Application/arduino-1.0.1/hardware/tools/avrdude.conf 
    -v -v -v -v 
    -c arduino -P /dev/ttyUSB0 -b 57600
    -D
    -p atmega328p

XXX:Upload usbasp bootloader using JeeNode ISP?::

    avrdude -v -p m8 -c usbasp 
      -U hfuse:w:0xC8:m -U lfuse:w:0xBF:m -U lock:w:0x0F:m
      -U flash:w:firmware/betemcu-usbasp/alternate_USBaspLoader_betemcu_timeout.hex

XXX: Using usbasp::
  
  avrdude -C/home/berend/Application/arduino-1.0.1/hardware/tools/avrdude.conf -v
    -v -v -v -patmega8 -carduino -P/dev/ttyUSB0 -b19200 -D
    -Uflash:w:/tmp/build7947190257849781585.tmp/atmega8l_usb.cpp.hex:i 

So, in summary the serial settings:

=================== ======== ==================
Protocol            Baudrate Device
=================== ======== ==================
Arduino (328/2009)  57600    USB BUB II
Arduino ISP         19200    JeeNode
Uno (optiboot?)     115200    
=================== ======== ==================

I'm using 57600, 3.3v, 16Mhz whenever I can in my homebuilds.

That means being stuck on 328P chips while I guess lower values are needed
on m8, m16, m32, m48, tiny85 are easily and cheaply available and I plan to 
experiment with. 

Remember running ATmega328P at 3.3v/16Mhz is out of specs already.

.. [#] <http://jeelabs.net/projects/hardware/wiki/JeeNode>

Device ID's
_____________

And also fuses and bootloader file for various devices.

=================== ============== ================= ===== ======
Board               U1 Device ID   Fuses             Lock  Unlock
=================== ============== ================= ===== ======
                                   Low   High  Ext    
=================== ============== ===== ===== ===== ===== ======
Arduino 328/2009    0x1e950f       0xFF  0xDE  0x05  0x    0x  
Arduino UNO         "              0xFF  0XDE  0x05  0x    0x  
JeeNode m328p       "              0x    0X    0x05  0x    0x  
Arduino 32          0x1e950e       0xE1  0X99  0x05  0x    0x  
Arduino 48          0x1e920a       0x    0X    0x    0x    0x  
48A                 0x1e9205       0x    0X    0x    0x    0x  
USBisp m8           0x1e9307       0xCF  0xBF  -     0x3C  0x  
eBay Sanguino 1284  0x             0x    0x    0x    0x    0x  
=================== ============== ===== ===== ===== ===== ======

Boards
------
uC16A
  - First prototype.
uC32A
  - First sanguino footprint module.
  - XXX: Need to check if it confirms or has analog reversed.
Cassette328P
  - First Arduino clone board, in old tape cassette.

Module support
--------------
Notes on individual plugs and modules. 

USBisp ``mx-usbisp-v3.00``
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

USBasp ``betemcu-usbasp-miniprog`` MiniProg
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

betemcu-usbasp/usbasp.2011-05-28/bin/firmware/usbasp.atmega8.2011-05-28.hex
  Working bootloader
  
atmega8_mkjdz.com_I2C_lcd1602.hex
  Program data to run I2C LCD demo on USBasp 

ArduinoISP_mega328.hex
  Arduino as ISP.

isp_flash_m328p.hex
  Run a JeeNode as Arduino ISP (with the flash board).

vs-32.hex
  Vectorscope image for atmega32, display adafruit image.
  X-axis on port A, Y-axis on C.

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


-----



My Boards
  1. Atmega16 test
  2. Atmega32 Dual Inline board
     - Upload bootloader OK. 
       Not sure about fuses.
       Cannot get serial upload.

  3. Atmega48 Cassette Board
  4. Atmega328 Cassette Board


ATmegaBOOT.hex                          16.000    19200  atmega8
ATmegaBOOT_168_ng.hex                   16.000    19200
ATmegaBOOT_168_diecimila.hex            16.000    19200  atmega168
ATmegaBOOT_168_pro_8MHz.hex              8.000    19200
ATmegaBOOT_168_atmega328.hex            16.000    57600
ATmegaBOOT_168_atmega328_bt.hex         16.000    19200  
ATmegaBOOT_168_atmega328_pro_8MHz.hex    8.000    57600
ATmegaBOOT_168_atmega1280.hex           16.000    57600  atmega1280
LilyPadBOOT_168.hex                      8.000    19200  
optiboot_atmega328.hex                  16.000   115200
optiboot_atmega328-Mini.hex             16.000   115200  

  
