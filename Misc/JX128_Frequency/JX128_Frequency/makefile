# Tools and directories
CC		= avr-gcc
AS		= avr-gcc -x assembler-with-cpp	
RM		= rm -f
RN		= mv
BIN		= avr-objcopy
INCDIR	= .
LIBDIR	= $(AVR)\avr\lib
SHELL   = $(AVR)\bin\sh.exe
FORMAT  = srec



	
# CPU type
MCU = atmega128

# Target
TRG	= main

# C-source files
SRC	= serial.c printf.c lcd.c $(TRG).c


# Libraries 
#LIB =  

# Compiler flags
CPFLAGS	=  -g -Os -fregmove -fforce-addr -Wall -Wstrict-prototypes -Wa,-ahlms=$(<:.c=.lst) -mcall-prologues -I$LIB


# Assembler flags
ASFLAGS = -Wa,-gstabs

# Linker flags
LDFLAGS = -Wl,--defsym,__init_wdtcr__=0x07,--defsym,__init_mcucr__=0x80,-Map=$(TRG).map,--cref

#---------------- Library Options ----------------
# Minimalistic printf version
PRINTF_LIB_MIN = -Wl,-u,vfprintf -lprintf_min

# Floating point printf version (requires MATH_LIB = -lm below)
PRINTF_LIB_FLOAT = -Wl,-u,vfprintf -lprintf_flt

# If this is left blank, then it will use the Standard printf version.
PRINTF_LIB = 
#PRINTF_LIB = $(PRINTF_LIB_MIN)
PRINTF_LIB = $(PRINTF_LIB_FLOAT)

#--------------------------------------------------------------------------
# Minimalistic scanf version 
SCANF_LIB_MIN = -Wl,-u,vfscanf -lscanf_min 

# Floating point + %[ scanf version (requires MATH_LIB = -lm below) 
SCANF_LIB_FLOAT = -Wl,-u,vfscanf -lscanf_flt 

# If this is left blank, then it will use the Standard scanf version. 
SCANF_LIB = 
SCANF_LIB = $(SCANF_LIB_FLOAT) 


MATH_LIB = -lm 

############################################################################################
############################################################################################

#define all project specific object files
	OBJ	= $(ASRC:.s=.o) $(SRC:.c=.o)
	CPFLAGS += -mmcu=$(MCU)
	ASFLAGS += -mmcu=$(MCU)
	LDFLAGS += -mmcu=$(MCU)
	LDFLAGS += $(PRINTF_LIB) $(SCANF_LIB) $(MATH_LIB) 

 
#this defines the aims of the make process
all:	$(TRG).elf $(TRG).hex 


#compile: instructions to create assembler and/or object files from C source
%o : %c 
	$(CC) -c $(CPFLAGS) -I$(INCDIR) $< -o $@

%s : %c
	$(CC) -S $(CPFLAGS) -I$(INCDIR) $< -o $@

#assemble: instructions to create object file from assembler files
%o : %s
	$(AS) -c $(ASFLAGS) -I$(INCDIR) $< -o $@


#link: instructions to create elf output file from object files
%elf: $(OBJ)
	$(CC) $(OBJ) $(LIB) $(LDFLAGS) -o $@

#create avrobj file from elf output file
%obj: %elf
	$(BIN) -O avrobj $< $@

#create bin (ihex, srec) file from elf output file
%rom: %elf
	$(BIN) -O $(FORMAT) $< $@

%hex: %elf
	$(BIN) -O ihex $< $@


	avr-size $(TRG).elf


#make instruction to delete created files
clean:
	$(RM) $(OBJ)
	$(RM) $(TRG).map
	$(RM) $(TRG).elf
	$(RM) $(TRG).obj
	$(RM) $(TRG).hex
	$(RM) *.bak
	$(RM) *.lst
	$(RM) ../*.lst
	$(RM) *.?_sym

clean_before:
	$(RM) $(TRG).hex
	$(RM) *.err

