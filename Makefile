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

FRAMEWORK_DIR = dsp_framework
WRAPPER_DIR   = dsp_wrapper
UNIT_TEST_DIR = unit_test
RELEASE_DIR = release

CFLAGS  = -O3

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
endif

ifeq ($(TIME_PROFILE), 1)
	CFLAGS += -DTIME_PROFILE
endif

all: DSP_FRAMEWORK
	echo "--- Build all dsp library ---"

DSP_FRAMEWORK: $(FRAMEWORK_DIR)
	echo "--- Build DSP framework ---"
	make -C $(FRAMEWORK_DIR)
	cp ./$(FRAMEWORK_DIR)/*.bin $(RELEASE_DIR)

DSP_WRAPPER: $(WRAPPER_DIR)
	echo "--- Build DSP wrapper ---"
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
	@echo "\tDSP_FRAMEWORK\t- build DSP framework"
	@echo "\tDSP_WRAPPER\t- build DSP wrapper"
	@echo "\tUNIT_TEST\t- build DSP unit test"
	@echo "\tall\t\t- build the above"

clean:
	make -C $(FRAMEWORK_DIR) clean
	rm -rf ./$(RELEASE_DIR)/*.so
	rm -rf ./$(RELEASE_DIR)/exe
	rm -rf ./$(RELEASE_DIR)/wrapper
	rm -rf ./$(RELEASE_DIR)/*.bin
