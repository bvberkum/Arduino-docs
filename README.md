## Arduino-docs
##### Master: [ ![master branch CI status](https://travis-ci.org/dotmpe/Arduino-docs.svg) ](https://travis-ci.org/dotmpe/Arduino-docs) Dev: [ ![dev branch CI status](https://travis-ci.org/dotmpe/Arduino-docs.svg?branch=dev) ](https://travis-ci.org/dotmpe/Arduino-docs/branches)
My Arduino sketch folder 

- My first "Arduino's" where BB atmega328P, [elm-chan-style](http://elm-chan.org/docs/wire/wiring_e.html) TQFP, and one [mega1284p](http://forum.freetronics.com/viewtopic.php?t=330).
- The first modules I bought where JeeNode's [JeeLabs](./JeeLabs/main.rst)
  and a collection of JeePort modules

* [Main doc](main.rst) for notes on "headless" build setup. 

* [Prototype](Prototype/main.rst) notes on using GIT history with derived files.
* [Blink](Prototype/Blink) is always a usefull starting point

----

[Prototype/TempGuard](./Prototype/TempGuard/main.rst)
[Mpe/Cassette328P](./Mpe/Cassette328P/main.rst)
[Mpe/Cassette328P/](./Mpe/Cassette328P/Cassette328P.rst)
[Misc/LCFrequencyMeter/](./Misc/LCFrequencyMeter/LCFrequencyMeter.rst)
[Misc/SinePWM/](./Misc/SinePWM/SinePWM.rst)
[Misc/nokia_pcd8544_display_testing](./Misc/nokia_pcd8544_display_testing/main.rst)

Other notes:

- [Firmware](./firmware/main.rst) refs and backups
- Some ``boards.txt`` fragments in [hardware](./hardware/main.rst)
- If needed, notes on [libraries](./libraries/main.rst)

[Misc](./Misc/main.rst)

----

With `arduino-cli`
```sh

  arduino-cli compile -b arduino:avr:pro --build-properties build.extra_flags=-DBLINK_PIN=13 Prototype/Blink
  arduino-cli upload -v -b arduino:avr:pro -p /dev/ttyUSB1 Prototype/Blink
```
