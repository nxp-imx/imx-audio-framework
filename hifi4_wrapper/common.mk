###############################################################
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
###############################################################

        #TOOL_CHAIN_PATH = /vobs/fsl_mad_linux_toolchains/gcc-4.1.1-glibc-2.4-nptl-6/arm-none-linux-gnueabi
        #TOOL_CHAIN_PATH = /opt/freescale/usr/local/gcc-4.1.2-glibc-2.5-nptl-3/arm-none-linux-gnueabi
#        TOOL_CHAIN_PATH = /usr
#        #CC              = $(TOOL_CHAIN_PATH)/bin/arm-none-linux-gnueabi-gcc
#        CC              = $(TOOL_CHAIN_PATH)/bin/arm-linux-gnueabi-gcc
#        #AR              = $(TOOL_CHAIN_PATH)/bin/arm-none-linux-gnueabi-ar
#        AR              = $(TOOL_CHAIN_PATH)/bin/arm-linux-gnueabi-ar
#        #LD              = $(TOOL_CHAIN_PATH)/bin/arm-none-linux-gnueabi-gcc
#        LD              = $(TOOL_CHAIN_PATH)/bin/arm-linux-gnueabi-gcc
#        #STDLIBDIR       = -L$(TOOL_CHAIN_PATH)/lib/gcc/arm-none-linux-gnueabi/4.1.2
#        STDLIBDIR       = -L$(TOOL_CHAIN_PATH)/lib/gcc/arm-linux-gnueabi/4.6.3
ifeq ($(BUILD), ARMV8ELINUX)
	TOOL_CHAIN_PATH   = /usr/gcc-linaro-4.9-2015.02-3-x86_64_aarch64-linux-gnu
	CC          = $(CCACHE) $(TOOL_CHAIN_PATH)/bin/aarch64-linux-gnu-gcc
	AR          = $(TOOL_CHAIN_PATH)/bin/aarch64-linux-gnu-ar
	LD          = $(CCACHE) $(TOOL_CHAIN_PATH)/bin/aarch64-linux-gnu-gcc
	STRIP       = $(TOOL_CHAIN_PATH)/bin/aarch64-linux-gnu-strip
	STDLIBDIR   = -L$(TOOL_CHAIN_PATH)/lib/gcc/aarch64-linux-gnu/4.9.3
	LIB_DIR     = ../release/wrapper
else
	TOOL_CHAIN_PATH = /opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi
	CC              = $(CCACHE) $(TOOL_CHAIN_PATH)/bin/arm-fsl-linux-gnueabi-gcc
	AR              = $(TOOL_CHAIN_PATH)/bin/arm-fsl-linux-gnueabi-ar
	LD              = $(CCACHE) $(TOOL_CHAIN_PATH)/bin/arm-fsl-linux-gnueabi-gcc
	STRIP           = $(TOOL_CHAIN_PATH)/bin/arm-fsl-linux-gnueabi-strip
	STDLIBDIR       = -L$(TOOL_CHAIN_PATH)/lib/gcc/arm-fsl-linux-gnueabi/3.4.4
	LIB_DIR         = ../release/wrapper
endif

ifeq ($(BUILD),ARM11LERVDS)
	TOOL_CHAIN_PATH = /vobs/fsl_mad_multimedia_tools/rvds_lin_3.0_sp/RVCT/Programs/3.0/586/linux-pentium/
	CC  = $(TOOLS_DIR)armcc
	AS  = $(TOOLS_DIR)armasm
	LD  = $(TOOLS_DIR)armlink
	AR  = $(TOOLS_DIR)armar

	CFLAGS = --cpu ARM1136J-S --littleend -O0 --debug --dwarf2 --fpu SoftVFP --enum_is_int

	TGT_OS_BIN = arm11_lervds
#       C_DEFINES  += -march=armv6
	TGT_OS = LERVDS
	TGT_CPU = ARM11
endif

ifeq ($(BUILD),ARM12LERVDS)
	TOOL_CHAIN_PATH = /vobs/fsl_mad_multimedia_tools/rvds_lin_3.0_sp/RVCT/Programs/3.0/586/linux-pentium/
	CC  = $(TOOLS_DIR)armcc
	AS  = $(TOOLS_DIR)armasm
	LD  = $(TOOLS_DIR)armlink
	AR  = $(TOOLS_DIR)armar

	CFLAGS = --cpu Cortex-A8 --littleend -O0 --debug --dwarf2 --fpu SoftVFP --enum_is_int

	TGT_OS_BIN = arm12_lervds
	TGT_OS = LERVDS
	TGT_CPU = ARM12
	C_DEFINES  += -march=armv6
endif

ifeq ($(BUILD),ARMV8ELINUX)
	TGT_OS_BIN = arm_elinux
	TGT_OS = ELINUX
	TGT_CPU = ARMV8
endif

ifeq ($(BUILD),ARM12ELINUX)
	TGT_OS_BIN = arm12_elinux
	TGT_OS = ELINUX
	TGT_CPU = ARM12
	C_DEFINES  += -march=armv6
endif

ifeq ($(BUILD),ARM11ELINUX)
	TGT_OS_BIN = arm11_elinux
	TGT_OS = ELINUX
	TGT_CPU = ARM11
	C_DEFINES  += -march=armv6
endif

ifeq ($(BUILD),ARM9ELINUX)
	TGT_OS_BIN = arm9_elinux
	TGT_OS = ELINUX
	TGT_CPU = ARM9
	C_DEFINES  += -march=armv5te
	C_DEFINES  += -mcpu=arm926ej-s
endif

ifeq ($(BUILD),ARM9ANDROID)
	ANDROID_BUILD = 1
ifdef NEW_ANDROID_NAME
	TGT_OS_BIN = arm9_android
else
	TGT_OS_BIN = arm9_elinux
endif
	TGT_OS     = ANDROID
	TGT_CPU    = ARM9
	C_DEFINES += -march=armv5te
endif

ifeq ($(BUILD),ARM11ANDROID)
	ANDROID_BUILD = 1
ifdef NEW_ANDROID_NAME
	TGT_OS_BIN = arm11_android
else
	TGT_OS_BIN = arm11_elinux
endif
	TGT_OS     = ANDROID
	TGT_CPU    = ARM11
	C_DEFINES += -march=armv7-a
endif

ifeq ($(BUILD),ARM12ANDROID)
	ANDROID_BUILD = 1
ifdef NEW_ANDROID_NAME
	TGT_OS_BIN = arm12_android
else
	TGT_OS_BIN = arm12_elinux
endif
	TGT_OS     = ANDROID
	TGT_CPU    = ARM12
	C_DEFINES += -march=armv7-a
endif

ifeq ($(BUILD),ARMV8ANDROID)
	ANDROID_BUILD = 1
	TGT_OS_BIN = arm_android
	TGT_OS     = ANDROID
	TGT_CPU    = ARMV8
endif

ifeq ($(ANDROID_BUILD),1)
        ifeq ($(TGT_CPU), ARMV8)
	TOOL_CHAIN_PATH = /opt/android-ndk64-r10e-standalone
	CC              = $(CCACHE) $(TOOL_CHAIN_PATH)/bin/aarch64-linux-android-gcc
	AR              = $(TOOL_CHAIN_PATH)/bin/aarch64-linux-android-ar
	LD              = $(CCACHE) $(TOOL_CHAIN_PATH)/bin/aarch64-linux-android-g++
	STRIP           = $(TOOL_CHAIN_PATH)/bin/aarch64-linux-android-strip
	STDLIBDIR       = -L$(TOOL_CHAIN_PATH)/lib/gcc/aarch64-linux-android/4.9 -L$(TOOL_CHAIN_PATH)/sysroot/usr/lib
	C_DEFINES      += -Wa,--noexecstack -Werror=format-security -fno-short-enums
	C_DEFINES      += -Wno-psabi
	C_DEFINES      += -W -Wall -Wno-unused -Winit-self -Wpointer-arith -Werror=return-type -Werror=non-virtual-dtor -Werror=address -Werror=sequence-point
	C_DEFINES      += -Wstrict-aliasing=2
	C_DEFINES      += -fno-exceptions -Wno-multichar
	C_DEFINES      += -fpic -ffunction-sections -funwind-tables -fstack-protector
	C_DEFINES      += -fmessage-length=0
	C_DEFINES      += -finline-functions -fno-inline-functions-called-once -fgcse-after-reload -frerun-cse-after-loop -frename-registers
	C_DEFINES      += -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64
	C_DEFINES      += -DANDROID -DNDEBUG -DNDEBUG -UDEBUG -MD -I$(TOOL_CHAIN_PATH)/sysroot/usr/include
	C_DEFINES      += -g -MD

	L_DEFINES      += -llog -lc -lstdc++ -lm -Wl,--gc-sections -Wl,-shared,-Bsymbolic -Wl,--whole-archive -Wl,--no-whole-archive -Wl,-z,noexecstack -Wl,--no-undefined

	LIB_DIR    =   ../release/wrapper
	OPTIMIZE        = -Os
        else
	TOOL_CHAIN_PATH = /opt/android-ndk-r9-standalone
	CC              = $(CCACHE) $(TOOL_CHAIN_PATH)/bin/arm-linux-androideabi-gcc
	AR              = $(TOOL_CHAIN_PATH)/bin/arm-linux-androideabi-ar
	LD              = $(CCACHE) $(TOOL_CHAIN_PATH)/bin/arm-linux-androideabi-g++
	STRIP           = $(TOOL_CHAIN_PATH)/bin/arm-linux-androideabi-strip
	STDLIBDIR       = -L$(TOOL_CHAIN_PATH)/lib/gcc/arm-linux-androideabi/4.6 -L$(TOOL_CHAIN_PATH)/sysroot/usr/lib
	C_DEFINES       +=  -mthumb -mthumb-interwork -mfpu=neon -mfloat-abi=softfp -msoft-float
	C_DEFINES      += -Wa,--noexecstack -Werror=format-security -fno-short-enums
	C_DEFINES      += -Wno-psabi
	C_DEFINES      += -W -Wall -Wno-unused -Winit-self -Wpointer-arith -Werror=return-type -Werror=non-virtual-dtor -Werror=address -Werror=sequence-point
	C_DEFINES      += -Wstrict-aliasing=2
	C_DEFINES      += -fno-exceptions -Wno-multichar -msoft-float
	C_DEFINES      += -fpic -ffunction-sections -funwind-tables -fstack-protector
	C_DEFINES      += -fmessage-length=0
	C_DEFINES      += -finline-functions -fno-inline-functions-called-once -fgcse-after-reload -frerun-cse-after-loop -frename-registers
	C_DEFINES      += -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64
	C_DEFINES      += -DANDROID -DNDEBUG -DNDEBUG -UDEBUG -MD -I$(TOOL_CHAIN_PATH)/sysroot/usr/include
	C_DEFINES      += -g -MD

	L_DEFINES      += -llog -lc -lstdc++ -lm -Wl,--gc-sections -Wl,-shared,-Bsymbolic -Wl,--whole-archive -Wl,--no-whole-archive -Wl,-z,noexecstack -Wl,--fix-cortex-a8 -Wl,--no-undefined

	LIB_DIR    =   ../release/wrapper
	OPTIMIZE        = -Os
        endif
endif

ifeq ($(BUILD),UNIX)
	TGT_OS_BIN 	= x86_unix
	TGT_OS          = UNIX
	TGT_CPU         = X86
	CC 		= $(CCACHE) gcc
	AR 		= ar
	LD 		= ld
endif

ifeq ($(BUILD),X86ELINUX)
	TGT_OS_BIN 	= x86_elinux
	TGT_OS          = ELINUX
	TGT_CPU         = X86
	CC 		= $(CCACHE) gcc
	AR 		= ar
	LD 		= $(CCACHE) gcc
endif

MEM_DEBUG  =NONE

ifndef OPTIMIZE
#	OPTIMIZE = -O2
endif

ifndef DEBUG
	DEBUG =
else
	C_DEFINES += -g -DDEBUG
endif

#Enable debug message?
C_DEFINES += -D_LINUX


