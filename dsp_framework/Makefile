# Copyright 2018 NXP
#

#***************************************************************************
#
# DSP Framework Makefile
#
# **************************************************************************

CC          = $(CCACHE) xt-xcc
CPLUS       = $(CCACHE) xt-xc++
OBJCOPY     = xt-objcopy
PKG_LOADLIB = xt-pkg-loadlib
XTENSA_CORE = hifi4_nxp_v4_3_1_3_dev

CFLAGS  = -O3 -DHAVE_XOS
CFLAGS += -DXA_FSL_UNIA_CODEC
TOOL_PATH   := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))../../imx-audio-toolchain/Xtensa_Tool

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
endif

ifeq ($(TIME_PROFILE), 1)
	CFLAGS += -DTIME_PROFILE
endif

ifeq ($(PLATF), imx8m)
	XTENSA_CORE = hifi4_mscale_v1_0_2_prod
	CFLAGS += -DPLATF_8M
endif

ifeq ($(PLATF), imx8ulp)
	XTENSA_CORE = hifi4_nxp2_s7_v1_1a_prod
	CFLAGS += -DPLATF_8ULP
endif

ROOT_DIR    = ../
SRC_DIR     = ./
INC_DIR     = ./

LINK_DIR    = ./memmap/$(PLATF)
SYSTEM_DIR  = $(TOOL_PATH)/builds/RI-2020.4-linux/$(XTENSA_CORE)/config

CFLAGS      += -mlongcalls -fmessage-length=0 -DPROC_$(XTENSA_CORE)       \
			  -DCONFIG_$(XTENSA_CORE) \
			  --xtensa-core=$(XTENSA_CORE)                                         \
			  --xtensa-system=$(SYSTEM_DIR)

CPLUS_FLAGS = -mlsp=$(LINK_DIR) --xtensa-system=$(SYSTEM_DIR) --xtensa-core=$(XTENSA_CORE)

INCLUDES    = -I$(INC_DIR)

INCLUDES   +=	-I$(INC_DIR)/include							\
		-I$(INC_DIR)/include/driver						\
		-I$(ROOT_DIR)/rpmsg-lite/lib/include					\
		-I$(ROOT_DIR)/rpmsg-lite/lib/include/environment/xos			\
		-I$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/include   		\
		-I$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/include/audio		\
		-I$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/include/sys/xos-msgq	\
		-I$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/include/sys/xos-msgq/iss	\
		-I$(ROOT_DIR)/libxa_af_hostless/algo/xa_af_hostless/include		\
		-I$(ROOT_DIR)/libxa_af_hostless/include					\
		-I$(ROOT_DIR)/libxa_af_hostless/include/audio				\
		-I$(ROOT_DIR)/libxa_af_hostless/include/sysdeps/xos/include		\
		-I$(ROOT_DIR)/testxa_af_hostless/test/include
INCLUDES   +=   -I$(ROOT_DIR)/common/include/fsl_unia
INCLUDES   +=   -I$(INC_DIR)/include/plugins
INCLUDES   +=   -I$(INC_DIR)/include/libloader
INCLUDES   +=   -I$(INC_DIR)/include/driver

# Put common C files here
C_OBJS  =	$(SRC_DIR)/src/board.o						\
		$(SRC_DIR)/src/fsl_msgq.o					\
		$(SRC_DIR)/src/main.o						\
		$(SRC_DIR)/src/memory.o						\
		$(SRC_DIR)/src/printf.o						\
		$(SRC_DIR)/src/rpmsg_env_bm.o					\
		$(SRC_DIR)/src/rpmsg_platform.o					\
		$(SRC_DIR)/src/rsc_table.o					\
		$(SRC_DIR)/src/driver/fsl_lpuart.o				\
		$(SRC_DIR)/src/driver/fsl_mu.o					\
		$(SRC_DIR)/src/driver/fsl_uart.o				\
		$(SRC_DIR)/src/driver/io.o					\
		$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/xf-msg.o	\
		$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/xf-msgq.o	\
		$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/xf-core.o	\
		$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/xf-mem.o	\
		$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/xf-sched.o	\
		$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/rbtree.o	\
		$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/xa-class-audio-codec.o	\
		$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/xa-class-renderer.o	\
		$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/xf-isr.o		\
		$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/xf-io.o		\
		$(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/xa-class-base.o	\
		$(ROOT_DIR)/testxa_af_hostless/test/plugins/xa-factory.o	\
		$(ROOT_DIR)/rpmsg-lite/lib/rpmsg_lite/rpmsg_lite.o		\
		$(ROOT_DIR)/rpmsg-lite/lib/rpmsg_lite/rpmsg_ns.o		\
		$(ROOT_DIR)/rpmsg-lite/lib/virtio/virtqueue.o			\
		$(ROOT_DIR)/rpmsg-lite/lib/common/llist.o
C_OBJS  +=	$(SRC_DIR)/src/plugins/xa-unia-codec.o				\
		$(SRC_DIR)/src/plugins/xa-fsl-renderer.o			\
		$(SRC_DIR)/src/libloader/dpu_lib_load.o				\
		$(SRC_DIR)/src/libloader/common.o				\
		$(SRC_DIR)/src/libloader/pi_library_load.o			\
		$(SRC_DIR)/src/libloader/pi_relocate_lib.o 			\
		$(SRC_DIR)/src/driver/asrc.o					\
		$(SRC_DIR)/src/driver/easrc.o 					\
		$(SRC_DIR)/src/driver/edma.o 					\
		$(SRC_DIR)/src/driver/esai.o 					\
		$(SRC_DIR)/src/driver/i2c.o 					\
		$(SRC_DIR)/src/driver/irqstr.o 					\
		$(SRC_DIR)/src/driver/sai.o 					\
		$(SRC_DIR)/src/driver/sdma.o 					\
		$(SRC_DIR)/src/driver/wm8960.o 					\
		$(SRC_DIR)/src/driver/wrap_dma.o


TARGET = hifi4_imx8qmqxp.bin

ifeq ($(PLATF), imx8m)
	TARGET = hifi4_imx8mp.bin
endif

ifeq ($(PLATF), imx8mp_lpa)
	TARGET = hifi4_imx8mp_lpa.bin
endif

ifeq ($(PLATF), imx8ulp)
	TARGET = hifi4_imx8ulp.bin
endif

PLATF ?= imx8

all: $(C_OBJS)
	$(CPLUS) $(CPLUS_FLAGS) $(C_OBJS) -lxos -lxtutil -o dsp_framework
	$(OBJCOPY) --xtensa-core=$(XTENSA_CORE) -Ibinary -Obinary dsp_framework $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<


clean:
	rm -f $(SRC_DIR)/src/*.o
	rm -f $(SRC_DIR)/src/driver/*.o
	rm -f $(SRC_DIR)/src/libloader/*.o
	rm -f $(SRC_DIR)/src/plugins/*.o
	rm -f $(SRC_DIR)/src/hardware/*.o
	rm -f $(ROOT_DIR)/libxa_af_hostless/algo/hifi-dpf/src/*.o
	rm -f $(ROOT_DIR)/testxa_af_hostless/test/plugins/*.o
	rm -f $(ROOT_DIR)/rpmsg-lite/lib/rpmsg_lite/*.o
	rm -f $(ROOT_DIR)/rpmsg-lite/lib/virtio/*.o
	rm -f $(ROOT_DIR)/rpmsg-lite/lib/common/*.o
	rm -f ./dsp_framework

