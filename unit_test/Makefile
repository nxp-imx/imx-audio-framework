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

TOOLS_DIR   = /usr/gcc-linaro-4.9-2015.02-3-x86_64_aarch64-linux-gnu
CC          = $(TOOLS_DIR)/bin/aarch64-linux-gnu-gcc
LN          = $(TOOLS_DIR)/bin/aarch64-linux-gnu-gcc

CFLAGS      = -Wl,--hash-style=gnu

ifeq ($(TIME_PROFILE), 1)
	CPU_FREQ = 1200
	CFLAGS += -DTIME_PROFILE -DCPUFREQ=$(CPU_FREQ)
endif

INCLUDES    = -I$(INC_DIR) -I$(INCLUDE_DIR)

# Put common C files here
C_OBJS      = $(SRC_DIR)/common.o                         \
              $(SRC_DIR)/id3_tag_decode.o                 \
              $(SRC_DIR)/dsp_test.o



all: $(C_OBJS)
	$(CC) $(CFLAGS) $(C_OBJS) -o dsp_test

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<


clean:
	rm -f $(SRC_DIR)/*.o
	rm -f ./dsp_test

