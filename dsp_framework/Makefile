# Copyright 2018 NXP
#

#***************************************************************************
#
# DSP Framework Makefile
#
# **************************************************************************

SRC_DIR     = ./
INC_DIR     = ./include

LINK_DIR    = $(TOOL_PATH)/builds/RF-2016.4-linux/hifi4_nxp_v3_3_1_2_dev/xtensa-elf/lib/mainsim
SYSTEM_DIR  = $(TOOL_PATH)/builds/RF-2016.4-linux/hifi4_nxp_v3_3_1_2_dev/config

CFLAGS      += -mlongcalls -fmessage-length=0 -DPROC_hifi4_nxp_v3_3_1_2_dev       \
			  -DCONFIG_hifi4_nxp_v3_3_1_2_dev -DINCL_DEMO_DEC=0 -DPROC_hifi2_std     \
			  -DCONFIG_hifi2_std -DDPU_LOCAL_MEM_DIRECT_MAP -DINCL_MP3_DEC=0       \
			  --xtensa-core=$(XTENSA_CORE)                                         \
			  --xtensa-system=$(SYSTEM_DIR)

CPLUS_FLAGS = -mlsp=$(LINK_DIR) --xtensa-system=$(SYSTEM_DIR) --xtensa-core=$(XTENSA_CORE)

INCLUDES    = -I$(INC_DIR)

INCLUDES   += -I$(INC_DIR)/lib                            \
			  -I$(INC_DIR)/audio                          \
			  -I$(SRC_DIR)/arch                           \
			  -I$(SRC_DIR)/arch/libloader/include         \
			  -I$(SRC_DIR)/plugins/audio_codec            \
			  -I$(SRC_DIR)/plugins/renderer            \
			  -I$(SRC_DIR)/component/audio \
			  -I$(SRC_DIR)/plugins/audio_codec/uni_audio

# Put common C files here
C_OBJS      = $(SRC_DIR)/arch/peripheral.o                               \
			  $(SRC_DIR)/arch/printf.o                                   \
			  $(SRC_DIR)/arch/dpu_lib_load.o                             \
			  $(SRC_DIR)/arch/libloader/common.o                         \
			  $(SRC_DIR)/arch/libloader/pi_library_load.o                \
			  $(SRC_DIR)/arch/libloader/pi_relocate_lib.o                \
			  $(SRC_DIR)/component/xa-factory.o                          \
			  $(SRC_DIR)/component/audio/xa-class-audio-codec.o          \
			  $(SRC_DIR)/component/audio/xa-class-base.o                 \
			  $(SRC_DIR)/component/audio/xa-class-renderer.o             \
			  $(SRC_DIR)/plugins/audio_codec/xa-unia-codec.o             \
			  $(SRC_DIR)/plugins/renderer/xa-renderer-esai.o             \
			  $(SRC_DIR)/plugins/renderer/io.o                           \
			  $(SRC_DIR)/plugins/renderer/asrc.o                         \
			  $(SRC_DIR)/plugins/renderer/edma.o                         \
			  $(SRC_DIR)/plugins/renderer/sai.o                          \
			  $(SRC_DIR)/plugins/renderer/esai.o                         \
			  $(SRC_DIR)/plugins/renderer/irqstr.o                       \
			  $(SRC_DIR)/init/main.o                                     \
			  $(SRC_DIR)/ipc/xf-shmem.o                                  \
			  $(SRC_DIR)/ipc/xf-msg.o                                    \
			  $(SRC_DIR)/memory/memory.o                                 \
			  $(SRC_DIR)/runtime/xf-core.o                               \
			  $(SRC_DIR)/runtime/xf-io.o                                 \
			  $(SRC_DIR)/runtime/xf-sched.o                              \
			  $(SRC_DIR)/runtime/util/rbtree.o


all: $(C_OBJS)
	$(CPLUS) $(CPLUS_FLAGS) $(C_OBJS) -o dsp_framework
	$(OBJCOPY) --xtensa-core=$(XTENSA_CORE) -Ibinary -Obinary dsp_framework hifi4.bin

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<


clean:
	rm -f $(SRC_DIR)/arch/*.o
	rm -f $(SRC_DIR)/arch/libloader/*.o
	rm -f $(SRC_DIR)/component/*.o
	rm -f $(SRC_DIR)/component/audio/*.o
	rm -f $(SRC_DIR)/plugins/audio_codec/*.o
	rm -f $(SRC_DIR)/plugins/renderer/*.o
	rm -f $(SRC_DIR)/init/*.o
	rm -f $(SRC_DIR)/ipc/*.o
	rm -f $(SRC_DIR)/memory/*.o
	rm -f $(SRC_DIR)/runtime/*.o
	rm -f $(SRC_DIR)/runtime/util/*.o
	rm -f ./hifi4*

