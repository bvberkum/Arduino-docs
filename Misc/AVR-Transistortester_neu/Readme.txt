Die 2 Ordner "ATMega8" und "ATMega48" enthalten die kompilierten Programme für den jeweiligen Controller.
Die Datei TransistorTestNew.hex muss in den Flash programmiert werden 
Die Datei TransistorTestNew.eep muss ins EEPROM. Das bitte nicht vergessen!

Beim Selbstkompilieren muss nur in den Projektoptionen oder im Makefile der Controllertyp entsprechend
definiert werden. Bei der Mega48-Version werden dann einige Features (Widerstandsmessung, Messung
der Basis-Emitter-Spannung bei Bipolartransistoren, Messung der Gatekapazität bei Anreicherungs-MOSFETS und Kapazitätsmessung)
weggelassen, um das Programm auf die erforderliche Größe zu bringen.