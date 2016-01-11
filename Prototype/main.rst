Derived Sketches
  Want to derive files, make `ino` scan for sentinels and diffs.

  LinkNode wants to derive from RF12demo, and follow changes to JeeLib.
  But also wants to grow to an Mpe style loop, serial command, eeprom etc.
  Also, want to try to write nRF24 alternative versions to RFM12.

  see also git prototype

Prototype
  Node
    - AtmegaTemp
    - AtmegaSRAM
    - AtmegaEEPROM

  Serial
    - Node
    - TODO: get the inputparser working again.

  I2C
    Not realized. Perhaps an I2C replace for UART?

  SensorNode
    - Node
    - Serial
    - DallasTempBus
    - AdaDHT
    - Magnetometer

Others are coming along, should document how they fit in here,
ie. catagorize their functions in node.rst


GIT Prototype code merges?
--------------------------
Can create one from JeeLib using substree split: fetch JeeLib into this
repository, checkout latest to jeelib_<branch> and then split from there.
::

  git remote add jeelib git@github.com:jcw/jeelib.git && git fetch jeelib
  git branch jeelib_master jeelib/master

  git co -f jeelib_master
  git subtree split --squash --prefix=examples/RF12/RF12demo/ _jeelib_rf12demo
  git co -f dev
  git read-tree --prefix=Prototype/RadioLink -u _jeelib_rf12demo

Could wrap this in a script to handle it using standard GIT under the hood::

  prototype-init Prototype/RadioLink jeelib/master examples/RF12/RF12demo
  prototype-update Prototype/RadioLink


Programming connectors / Standard pinouts
-----------------------------------------

1. RxD
2. TxD

1. SCL
2. SDA

1. PWR

1. GND
2. RST
3. VCC
4. SCK
5. MISO
6. MOSI


