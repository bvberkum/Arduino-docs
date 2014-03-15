include                $(MK_SHARE)Core/Main.dirstack.mk
MK_$d               := $/Rules.mk
MK                  += $(MK_$d)
#
#      ------------ --

ifeq ($(OLD),1)
include Rules.old.mk
else
include Rules.new.mk
endif
include Rules.attiny.mk

#
#DIR                 := $/mydir
#include                $(call rules,$(DIR)/)
#
# DMK += $/dynamic-makefile.mk
# DEP += $/generated-dependency
# TRGT += $/build-target
# TEST += $/testtarget
# XXX: cleanable only noticed on 'make clean'
#ifneq ($(call contains,$(MAKECMDGOALS),clean),)
# XXX: using find here is so wastefull (otherwise), there is no cache at all?
CLN += $(shell find $/ -name .dep -or -name .lib -o -name *.o -o -name *.swp -o -name *.swo)
#endif

#      ------------ -- 
#
include                $(MK_SHARE)Core/Main.dirstack-pop.mk
# vim:noet:

