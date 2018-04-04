#*****************************************************************************
# Copyright 2018 NXP
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:

# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
#*****************************************************************************

include ./common.mk
VERSION = 1

# Library Name
PROGRAM	        = dsp_wrap
CSRCS_DIR       = src
OBJ_DIR         = object
INC_DIR         = include
COMM_SRC_DIR    = ../common

LIBRARY     = $(LIB_DIR)/lib_$(PROGRAM)_$(TGT_OS_BIN)
INCLUDES    = -I./include/uni_audio/
INCLUDES    += -I../include -I$(COMM_SRC_DIR)/libloader -I$(COMM_SRC_DIR)/proxy -I$(COMM_SRC_DIR)/api

CARM    =  $(CFLAGS_$(BUILD)) $(INCLUDES) $(OPTIMIZE) $(C_DEFINES)

ifeq ($(TGT_OS),ANDROID)
ifndef NEW_ANDROID_NAME
LIBRARY     = $(LIB_DIR)/lib_$(PROGRAM)_$(TGT_OS_BIN)_android
endif
CARM += -DTGT_OS_ANDROID
endif

ifeq ($(DEBUG),1)
        CARM    += -DDEBUG
endif

ifeq ($(BUILD),UNIX)
        CARM    += -fPIC
else
        CARM    +=   -fPIC
        CARM    +=   $(ENDIAN_$(TARGET_ENDIAN))
endif


# Put the C files here
C_OBJS	    = $(OBJ_DIR)/dsp_wrap.o
C_OBJS	   += $(OBJ_DIR)/dsp_dec.o           \
              $(OBJ_DIR)/xaf-api.o           \
              $(OBJ_DIR)/library_load.o      \
              $(OBJ_DIR)/xf-fio.o            \
              $(OBJ_DIR)/xf-proxy.o

SONAME = lib_$(PROGRAM)_$(TGT_OS_BIN).so.$(VERSION)

#Include only 'c' files for unix build
ifeq ($(BUILD),UNIX)
	OBJS = $(C_OBJS)
else
  OBJS = $(C_OBJS)
endif

all: 		create  LIB_$(TGT_CPU)_$(TGT_OS)
		@+echo "--- Build-all done ---"

# create the object directory
create:
		@echo "Creating directory" $(OBJDIR)
		@mkdir -p $(OBJ_DIR)

#Build the library

LIB_ARMV8_ANDROID	:$(OBJS)
			$(LD) -o $(LIB_ARGS) $(LIBRARY).so --shared -Wl,-soname,$(SONAME) -fPIC $(OBJS) $(STDLIBDIR) $(OPT_ASM_LIB) -lgcc -lm

LIB_ARM12_ANDROID	:$(OBJS)
			$(LD) -o $(LIB_ARGS) $(LIBRARY).so --shared -Wl,-soname,$(SONAME) -fPIC $(OBJS) $(STDLIBDIR) $(OPT_ASM_LIB) -lgcc -lm

LIB_ARMV8_ELINUX	:$(OBJS)
			$(LD) -o $(LIB_ARGS) $(LIBRARY).so --shared -Wl,-soname,$(SONAME) -fPIC $(OBJS) $(STDLIBDIR) $(OPT_ASM_LIB) -lgcc -lm

LIB_ARM12_ELINUX	:$(OBJS)
			$(LD) -o $(LIB_ARGS) $(LIBRARY).so --shared -Wl,-soname,$(SONAME) -fPIC $(OBJS) $(STDLIBDIR) $(OPT_ASM_LIB) -lgcc -lm

$(OBJ_DIR)/%.o:		$(CSRCS_DIR)/%.c
			$(CC) $(CARM) -I$(INC_DIR) -c  -o $@ $<

$(OBJ_DIR)/%.o:		$(COMM_SRC_DIR)/libloader/%.c
			$(CC) $(CARM) -I$(INC_DIR) -c  -o $@ $<

$(OBJ_DIR)/%.o:		$(COMM_SRC_DIR)/proxy/%.c
			$(CC) $(CARM) -I$(INC_DIR) -c  -o $@ $<

$(OBJ_DIR)/%.o:		$(COMM_SRC_DIR)/api/%.c
			$(CC) $(CARM) -I$(INC_DIR) -c  -o $@ $<

clean:
		rm -f $(OBJS) *~ core
		rm -f $(LIBRARY).a
		rm -f $(LIBRARY).so

usage:
	@echo "--------------------------------------------------------------"
	@echo "           Makefile for DSP  Wrapper Library "
	@echo "--------------------------------------------------------------"
	@echo ""
	@echo "Usage: $(MAKE) [options] [targets] [flags]"
	@echo "        flags:"
	@echo "            clean - clean all"
	@echo "            BUILD=ARM12ANDROID  - clean ARM12ANDROID"
	@echo "            BUILD=ARMV8ELINUX - clean ARMV8ELINUX"
	@echo "            BUILD=ARMV8ANDROID  - clean ARMV8ANDROID"
	@echo "		 =ARM12ANDROID - builds for ARM12 "
	@echo "		 =ARMV8ELINUX  - builds for ARMV8 "
	@echo "		 =ARMV8ANDROID - builds for ARMV8 "
	@echo " "
