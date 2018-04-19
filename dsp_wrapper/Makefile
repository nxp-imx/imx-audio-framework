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

#*****************************************************************
# This is the makefile of  lib & test program.
# Usage:
#		make BUILD="build type"
# 	make clean BUILD="build type"
#
# Build types:
# 	ARM12ANDROID: armv7 CPU, ANDROID
# 	ARMV8ELINUX: armv8 CPU, LINUX
# 	ARMV8ANDROID: armv8 CPU, ANDROID
#
#	For definition of differenet build types, please refer to
# file "common.mk" under the same directory.
###############################################################

all: library
	@+echo "--- dsp wrapper lib"

library:
	$(MAKE) -f lib_dsp_wrap.mk

.PHONEY: clean

clean:
	$(MAKE) clean -f lib_dsp_wrap.mk

usage:
	@echo "--------------------------------------------------------------"
	@echo "           Makefile for dsp wrapper Library & Test Program "
	@echo "--------------------------------------------------------------"
	@echo ""
	@echo "Usage: $(MAKE) [options] [targets] [flags]"
	@echo "        flags:"
	@echo "            clean - clean all"
	@echo "            BUILD=ARM12ANDROID clean  - clean ARM12ANDROID"
	@echo "            BUILD=ARMV8ELINUX clean  - clean ARMV8ELINUX"
	@echo "            BUILD=ARMV8ANDROID clean  - clean ARMV8ANDROID"
	@echo "            BUILD=ARM12ANDROID  - builds for ARM12 "
	@echo "            BUILD=ARMV8ELINUX  - builds for ARMV8 "
	@echo "            BUILD=ARMV8ANDROID  - builds for ARMV8 "
	@echo " "
