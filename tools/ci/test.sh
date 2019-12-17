#!/bin/bash

set -e -o nounset -o pipefail

test -z "${DEBUG-}" || set -x


test -d ~/build/dotmpe/docker-arduino/.git && {
  ( cd ~/build/dotmpe/docker-arduino && git pull origin master )
} || {
  rm -rf ~/build/dotmpe/docker-arduino
  git clone https://github.com/dotmpe/docker-arduino ~/build/dotmpe/docker-arduino
}

ln -s ~/build/dotmpe/docker-arduino/exec.sh ~/bin/arduino

mkdir -vp $DCKR_VOL/arduino

( cd ~/build/dotmpe/docker-arduino &&
  . ./vars.sh &&
  ./init.sh )

arduino compile -b arduino:avr:pro Prototype/Blink
arduino compile -b arduino:avr:pro Prototype/Node
arduino compile -b arduino:avr:pro Prototype/AtmegaEEPROM

./tools/sh/ard.sh compile-all

set +o nounset
