#!/bin/bash

git clone http://github.com/dotmpe/docker-arduino ~/build/dotmpe/docker-arduino

ln -s ~/build/dotmpe/docker-arduino/exec.sh ~/bin/arduino

arduino core install arduino:avr

arduino compile -b arduino:avr:pro Prototype/Blink
arduino compile -b arduino:avr:pro Prototype/Node
arduino compile -b arduino:avr:pro Prototype/AtmegaEEPROM

for x in Prototype/*/
do
  arduino compile -b arduino:avr:pro $x || true
done
