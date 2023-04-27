# Copyright 2023 NXP
#

#***************************************************************************
#
# DSP voice audio library Makefile
#
#***************************************************************************

# export for sub-makes
export CCACHE=
ifeq ($(USE_CCACHE), 1)
	CCACHE=$(shell which ccache)
endif

CC          = $(CCACHE) xt-clang
CPLUS       = $(CCACHE) xt-clang++
OBJCOPY     = xt-objcopy
PKG_LOADLIB = xt-pkg-loadlib
XTENSA_CORE = hifi4_nxp_v5_3_1_prod

ifeq ($(PLATF), imx8m)
	XTENSA_CORE = hifi4_mscale_v2_0_2_prod
endif

ifeq ($(PLATF), imx8ulp)
	XTENSA_CORE = hifi4_nxp2_s7_v2_1a_prod
	CFLAGS += -DPLATF_8ULP
endif

TOOL_PATH   := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))../../imx-audio-toolchain/Xtensa_Tool

LINK_DIR    = ./memmap/$(PLATF)
SYSTEM_DIR  = $(TOOL_PATH)/builds/RI-2023.11-linux/$(XTENSA_CORE)/config

CFLAGS  = -O3

CFLAGS  += -mlongcalls -fmessage-length=0 -fPIC

XTENSA_FLAGS = 	--xtensa-core=$(XTENSA_CORE)   \
		--xtensa-system=$(SYSTEM_DIR)

LINK_FLAGS = 	-mlsp=pisplitload -Wl,--shared-pagesize=128 -Wl,-pie          \
		$(XTENSA_FLAGS) -lc

CFLAGS += $(XTENSA_FLAGS)

RELEASE_DIR = release

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG_BASE -DDEBUG_1
endif
ifeq ($(DEBUG), 2)
	CFLAGS += -DDEBUG_BASE -DDEBUG_2
endif

export CC CPLUS OBJCOPY XTENSA_CORE PKG_LOADLIB TOOL_PATH CFLAGS LINK_FLAGS

ROOT_DIR = ../
COM_DIR = $(ROOT_DIR)/common
SRC_DIR = ./src

INCLUDE_DIR = ./include

INCLUDES = -I$(INCLUDE_DIR) -I$(COM_DIR)/include/fsl_unia

VOICE_LIB = voice_process_dummy

C_OBJS  = $(SRC_DIR)/voice_process_wrap.o \
	  $(SRC_DIR)/voice_process_dummy.o

all: $(C_OBJS)
	echo "--- Build audio process library ---"
	$(CPLUS) $(C_OBJS) $(LINK_FLAGS) -o lib_dsp_$(VOICE_LIB).so

help:
	@echo "make all to build the library"

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -f $(C_OBJS)
	rm -rf ./*.so

