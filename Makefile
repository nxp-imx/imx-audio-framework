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
# DSP Makefile
#
#***************************************************************************

# export for sub-makes
export CCACHE=
ifeq ($(USE_CCACHE), 1)
	CCACHE=$(shell which ccache)
endif

CC          = $(CCACHE) xt-xcc
CPLUS       = $(CCACHE) xt-xc++
OBJCOPY     = xt-objcopy
PKG_LOADLIB = xt-pkg-loadlib
XTENSA_CORE = hifi4_nxp_v3_3_1_2_dev

TOOL_PATH   := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))fsl_mad_dsp_toolchain/Xtensa_Tool
FRAMEWORK_DIR = hifi4_framework
WRAPPER_DIR   = hifi4_wrapper
UNIT_TEST_DIR = unit_test
RELEASE_DIR = release

CFLAGS  = -O3

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
endif

ifeq ($(TIME_PROFILE), 1)
	CFLAGS += -DTIME_PROFILE
endif

export CC CPLUS OBJCOPY XTENSA_CORE PKG_LOADLIB TOOL_PATH CFLAGS

all: HIFI4_WRAPPER HIFI4_FRAMEWORK UNIT_TEST
	echo "--- Build all dsp library ---"

HIFI4_FRAMEWORK: $(FRAMEWORK_DIR)
	echo "--- Build hifi4 framework ---"
	make -C $(FRAMEWORK_DIR)
	cp ./$(FRAMEWORK_DIR)/hifi4.bin $(RELEASE_DIR)

HIFI4_WRAPPER: $(WRAPPER_DIR)
	echo "--- Build hifi4 wrapper ---"
	mkdir -p $(RELEASE_DIR)/wrapper
ifeq ($(ANDROID_BUILD), 1)
	make -C $(WRAPPER_DIR) BUILD=ARM12ANDROID NEW_ANDROID_NAME=1 clean all
	make -C $(WRAPPER_DIR) BUILD=ARMV8ANDROID NEW_ANDROID_NAME=1 clean all
else
	make -C $(WRAPPER_DIR) BUILD=ARMV8ELINUX clean all
endif

UNIT_TEST: $(UNIT_TEST_DIR)
	echo "--- Build unit test ---"
	mkdir -p $(RELEASE_DIR)/exe
ifeq ($(ANDROID_BUILD), 1)
	echo "--- no unit test for android ---"
else
	make -C $(UNIT_TEST_DIR)
	cp ./$(UNIT_TEST_DIR)/dsp_test $(RELEASE_DIR)/exe
endif

help:
	@echo "targets are:"
	@echo "\tHIFI4_FRAMEWORK\t- build HiFi4 framework"
	@echo "\tHIFI4_WRAPPER\t- build HiFi4 wrapper"
	@echo "\tUNIT_TEST\t- build HiFi4 unit test"
	@echo "\tall\t\t- build the above"

clean:
	rm -rf ./$(FRAMEWORK_DIR)/src/*.o
	rm -rf ./$(FRAMEWORK_DIR)/hifi4*
	rm -rf ./$(WRAPPER_DIR)/object
	rm -rf ./$(UNIT_TEST_DIR)/src/*.o
	rm -rf ./$(UNIT_TEST_DIR)/dsp_test
	rm -rf ./$(RELEASE_DIR)/*.so
	rm -rf ./$(RELEASE_DIR)/exe
	rm -rf ./$(RELEASE_DIR)/wrapper
	rm -rf ./$(RELEASE_DIR)/*.bin
