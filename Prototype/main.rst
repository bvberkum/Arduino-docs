Want to derive files.
Keep multiple templates called prototypes that somehow easily merge with
upstream changes.

Primary challenge (using GIT) should be to observe the proper protocols for
identifying where is upstream?

LinkNode wants to derive from RF12demo, and follow changes to JeeLib.
But also wants to grow to an Mpe style loop, serial command, eeprom etc.
Also, want to try to write nRF24 alternative versions to RFM12.

So right now there is:

  Node
    Serial
    I2C
    SensorNode
      ..
    DallasTempBus
      ..

But no Prototype for radio nodes yet.
