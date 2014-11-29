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

