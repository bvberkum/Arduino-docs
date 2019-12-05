Derived Sketches
  Want to derive files, make `ino` scan for sentinels and diffs.

  LinkNode wants to derive from RF12demo, and follow changes to JeeLib.
  But also wants to grow to an Mpe style loop, serial command, eeprom etc.
  Also, want to try to write nRF24 alternative versions to RFM12.

  see also .gitprototype files, and ino.tab ofcourse

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

This should squash examples/RF12/RF12demo/ deltas, at commit-ref _jeelib_rf12demo?
XXX: this should output the new commit ID
Readtree should get that new synthetic history, and add it to the index.

Could wrap this in a script to handle it using standard GIT under the hood::

  prototype-init Prototype/RadioLink jeelib/master examples/RF12/RF12demo
  prototype-update Prototype/RadioLink

So untested, it seems thats how the stupid content tracker 's
file-content versioning can be levereged into GIT+CI. Deltas in the form of
change-sets and merge commits can be tracked per file, and in different lines
called branches, tagged commits, etc. as per usual GIT commit graph.

So based on discrete files we can select and create a synthetic, new version
with subtree split, and apply that elsewhere using read-tree. The subject is
a file or directory, and hopefully the result at the target is a path that is
updated from one, or more upstream "files" (not just branche or tag, but
commitish plus path) and may/will have local changes as well.

This derived content tracking requires some Ids to be resolved automatically,
and maybe there is some validatation that can be done in between. This does not
solve the composite-file use case yet, how to go from several files into one
either means:

- building a full history of the (derived) file aggregation, if that works; or
- leverage ht-sync further (as planned) to sync blocks (like htd.sh lib
  functions diff/sync)

For .ino files the process is matching preproc lines and blocks with external
content. Don't fancy splitting it up right now, but maybe just need to do anyway
and compile static untracked build results instead.


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
