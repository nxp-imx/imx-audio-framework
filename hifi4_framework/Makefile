#*****************************************************************
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
#*****************************************************************

#***************************************************************************
#
# DSP Framework Makefile
#
# **************************************************************************

SRC_DIR     = ./src
INC_DIR     = ./include
INCLUDE_DIR = ../include

LINK_DIR    = $(TOOL_PATH)/builds/RF-2016.4-linux/hifi4_nxp_v3_3_1_2_dev/xtensa-elf/lib/mainsim
SYSTEM_DIR  = $(TOOL_PATH)/builds/RF-2016.4-linux/hifi4_nxp_v3_3_1_2_dev/config

CFLAGS      += -mlongcalls -fmessage-length=0 -DPROC_hifi4_nxp_v3_3_1_2_dev       \
			  -DCONFIG_hifi4_nxp_v3_3_1_2_dev -DINCL_DEMO_DEC=0 -DPROC_hifi2_std     \
			  -DCONFIG_hifi2_std -DDPU_LOCAL_MEM_DIRECT_MAP -DINCL_MP3_DEC=0       \
			  --xtensa-core=$(XTENSA_CORE)                                         \
			  --xtensa-system=$(SYSTEM_DIR)

CPLUS_FLAGS = -mlsp=$(LINK_DIR) --xtensa-system=$(SYSTEM_DIR) --xtensa-core=$(XTENSA_CORE)

INCLUDES    = -I$(INC_DIR) -I$(INCLUDE_DIR)


# Put common C files here
C_OBJS      = $(SRC_DIR)/common.o                         \
			  $(SRC_DIR)/memory.o                         \
			  $(SRC_DIR)/dpu_lib_load.o                   \
			  $(SRC_DIR)/icm_dpu.o                        \
			  $(SRC_DIR)/intr_utils.o                     \
			  $(SRC_DIR)/main.o                           \
			  $(SRC_DIR)/peripheral.o                           \
			  $(SRC_DIR)/printf.o                           \
			  $(SRC_DIR)/pi_library_load.o                \
			  $(SRC_DIR)/pi_relocate_lib.o                \
			  $(SRC_DIR)/process.o



all: $(C_OBJS)
	$(CPLUS) $(CPLUS_FLAGS) $(C_OBJS) -o hifi4_framework
	$(OBJCOPY) --xtensa-core=$(XTENSA_CORE) -Ibinary -Obinary hifi4_framework hifi4.bin

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<


clean:
	rm -f $(SRC_DIR)/*.o
	rm -f ./hifi4*

