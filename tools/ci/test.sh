#!/bin/bash

git clone http://github.com/dotmpe/docker-arduino ~/build/dotmpe/docker-arduino

echo $PATH

ln -s ~/build/dotmpe/docker-arduino/exec.sh ~/bin/arduino

arduino core install arduino:avr

arduino compile -b arduino:avr:pro Prototype/Blink
