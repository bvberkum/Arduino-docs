#!/bin/bash

set -e -o nounset -o pipefail -x

git clone http://github.com/dotmpe/docker-arduino ~/build/dotmpe/docker-arduino

ln -s ~/build/dotmpe/docker-arduino/exec.sh ~/bin/arduino

arduino core install arduino:avr

arduino compile -b arduino:avr:pro Prototype/Blink
arduino compile -b arduino:avr:pro Prototype/Node
arduino compile -b arduino:avr:pro Prototype/AtmegaEEPROM

./tools/sh/ard.sh compile-all
