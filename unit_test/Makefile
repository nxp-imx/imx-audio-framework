#*****************************************************************
# Copyright 2018-2022 NXP
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
COM_DIR     = ../common
ROOT_DIR    = ../

TOOLS_DIR   = /usr/gcc-linaro-4.9-2015.02-3-x86_64_aarch64-linux-gnu
CC          = $(TOOLS_DIR)/bin/aarch64-linux-gnu-gcc
LN          = $(TOOLS_DIR)/bin/aarch64-linux-gnu-gcc

CFLAGS      =	-Wl,--hash-style=gnu -lpthread			\
		-DHAVE_LINUX -DHIFI_ONLY_XAF \
		-DFIO_LOCAL_FS -DXA_DISABLE_DEPRECATED_API
CFLAGS     +=  -DXA_FSL_UNIA_CODEC

ifeq ($(DEBUG), 1)
	CFLAGS += -DXF_TRACE
endif

ifeq ($(TIME_PROFILE), 1)
	CPU_FREQ = 1200
	CFLAGS += -DXAF_PROFILE -DCPUFREQ=$(CPU_FREQ)
endif

INCLUDES  =	-I$(INC_DIR)								\
		-I$(ROOT_DIR)/libxa_af_hostless/algo/host-apf/include			\
		-I$(ROOT_DIR)/libxa_af_hostless/include					\
		-I$(ROOT_DIR)/libxa_af_hostless/include/audio				\
		-I$(ROOT_DIR)/libxa_af_hostless/include/sysdeps/linux/include		\
		-I$(ROOT_DIR)/libxa_af_hostless/algo/xa_af_hostless/include		\
		-I$(ROOT_DIR)/libxa_af_hostless/algo/host-apf/include/sys/linux-msgq	\
		-I$(ROOT_DIR)/testxa_af_hostless/test/include				\
		-I$(COM_DIR)/include                                                    \
		-I$(COM_DIR)/include/fsl_unia

# Put common C files here
C_OBJS  =	$(COM_DIR)/src/xaf-fsl-api.o					\
		$(COM_DIR)/src/xf-fsl-ipc.o					\
		$(SRC_DIR)/id3_tag_decode.o					\
		$(COM_DIR)/src/library_load.o					\
		$(ROOT_DIR)/libxa_af_hostless/algo/host-apf/src/xf-proxy.o	\
		$(ROOT_DIR)/libxa_af_hostless/algo/host-apf/src/xf-trace.o	\
		$(ROOT_DIR)/testxa_af_hostless/test/src/xaf-utils-test.o	\
		$(ROOT_DIR)/testxa_af_hostless/test/src/xaf-fio-test.o		\
		$(ROOT_DIR)/testxa_af_hostless/test/src/xaf-mem-test.o

C_OBJS_REND  =	$(C_OBJS) $(SRC_DIR)/xaf-fsl-mp3-dec-rend-test.o
OUT_REND     =	dsp_rend_test.out

C_OBJS_DEC  =	$(C_OBJS) $(SRC_DIR)/xaf-fsl-dec-test.o
OUT_DEC     =	dsp_test.out

ifeq ($(TFLM), 1)
INCLUDES	+=	-I$(SRC_DIR)/tflm \
				-I$(ROOT_DIR)/testxa_af_hostless/test/plugins/cadence/tflm_common

C_OBJS_TFLM =	$(C_OBJS) $(SRC_DIR)/dsp_tflm_test.o \
				$(SRC_DIR)/tflm/yes_1000ms_audio_data.o \
				$(SRC_DIR)/tflm/no_1000ms_audio_data.o
OUT_TFLM    =	dsp_tflm_test.out
endif

all: DEC REND TFLM

DEC: $(C_OBJS_DEC)
	$(CC) $(CFLAGS) $(C_OBJS_DEC) -o $(OUT_DEC)
REND: $(C_OBJS_REND)
	$(CC) $(CFLAGS) $(C_OBJS_REND) -o $(OUT_REND)

ifeq ($(TFLM), 1)
TFLM: $(C_OBJS_TFLM)
	$(CC) $(CFLAGS) $(C_OBJS_TFLM) -o $(OUT_TFLM)
else
TFLM:
endif

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.cc
	$(CC) -std=c++11 $(CFLAGS) $(INCLUDES) -c -o $@ $<


clean:
	rm -f $(SRC_DIR)/*.o
	rm -f $(ROOT_DIR)/libxa_af_hostless/algo/host-apf/src/*.o
	rm -f $(ROOT_DIR)/testxa_af_hostless/test/src/*.o
	rm -f $(COM_DIR)/src/*.o
	rm -f ./*.out

