include                $(MK_SHARE)Core/Main.dirstack.mk
MK_$d               := $/Rules.mk
MK                  += $(MK_$d)
#
#      ------------ --


ifeq ($(shell uname -s),Darwin)
ARDUINOAPPDIR := $(shell cd /Applications/Arduino.app/ && pwd -P)
else
ARDUINOAPPDIR := $(shell realpath ./arduinodir)
endif
$(info $(shell $(call log,header2,ARDUINOAPPDIR,$(ARDUINOAPPDIR))))

ifneq ($(wildcard $(ARDUINOAPPDIR)/Contents/Resources/Java),)
ARDUINODIR := $(ARDUINOAPPDIR)/Contents/Resources/Java
else
ifneq ($(wildcard $(ARDUINOAPPDIR)/Contents/Java),)
ARDUINODIR := $(ARDUINOAPPDIR)/Contents/Java
else
$(error No ARDUINODIR for $(ARDUINOAPPDIR))
endif
endif

$(info $(shell $(call log,header2,ARDUINODIR,$(ARDUINODIR))))

#      ------------ --

arduinodir-libraries-relink::
	for f in libraries/*; do \
		F=$$(basename $$f); \
		if [ -n "$$(echo $$F | grep '[^a-zA-Z0-9_]')" ] ; then \
			continue; fi; \
		T=$$(realpath arduinodir/libraries)/$$F; \
		[ -e "$$T" ] && [ -L "$$T" ] && { \
			rm -v $$T; \
		} || { \
			echo Existing library $$F; \
		};\
		[ ! -e "$$T" ] && { \
			ln -sv $$(pwd -P)/$$f $$T; \
		} || { \
			echo -n; \
		};\
	done


./Misc/Soarer_Converter/docs:
	unzip ./Misc/Soarer_Converter/Soarer_Converter_v1.12_docs.zip -c Misc/Soarer_Converter


#      ------------ --

include             $/Rules.serial.mk

#      ------------ --


OLD ?= 0
ifeq ($(OLD),1)
include             $/Rules.edams.mk
else
include             $/Rules.sudar.mk
endif

include             $/Rules.targets.mk

#include Rules.new.mk


#      ------------ --

ifneq ($(call contains,$(MAKECMDGOALS),clean),)
CLN += $(shell find $/ -name .dep -or -name .lib -o -name *.o -o -name *.swp -o -name *.swo)
endif

#DIR                 := $/mydir
#include                $(call rules,$(DIR)/)
#
# DMK += $/dynamic-makefile.mk
# DEP += $/generated-dependency
# TRGT += $/build-target
# TEST += $/testtarget
# XXX: cleanable only noticed on 'make clean'

#      ------------ -- 
#
include                $(MK_SHARE)Core/Main.dirstack-pop.mk
# vim:noet:
