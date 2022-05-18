SYSTEM_DIR    = $(TOOL_PATH)/builds/RI-2020.4-linux/$(XTENSA_CORE)/config
XTENSA_BASE   = $(TOOL_PATH)
XTENSA_SYSTEM = $(SYSTEM_DIR)
XTENSA_TOOLS_VERSION = "RI-2020.4-linux"

export XTENSA_BASE XTENSA_CORE XTENSA_TOOLS_VERSION

ifeq ($(PROPRIETARY), 1)
	KERNELS = "xtensa"
endif

CXX       = $(CCACHE) xt-clang++
CXX_FLAGS = -std=c++11 -stdlib=libc++ -DTF_LITE_STATIC_MEMORY -DTF_LITE_DISABLE_X86_NEON \
            -fno-rtti -fno-exceptions -fno-threadsafe-statics -fno-unwind-tables

CFLAGS += -DXA_TFLM_MICROSPEECH

INCLUDES    += -I$(SRC_DIR)/tflite-micro
INCLUDES    += -I$(SRC_DIR)/tflite-micro/tensorflow/lite/micro/tools/make/downloads/flatbuffers/include
INCLUDES    += -I$(SRC_DIR)/tflite-micro/tensorflow/lite/micro/tools/make/downloads/kissfft
INCLUDES    += -I$(ROOT_DIR)/testxa_af_hostless/test/include/audio
INCLUDES    += -I$(ROOT_DIR)/testxa_af_hostless/test/plugins/cadence/tflm_common
INCLUDES    += -I$(ROOT_DIR)/testxa_af_hostless/test/plugins/cadence/tflm_microspeech

TFLM_LIB = $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/tools/make/gen/xtensa_hifi4_default/lib/libtensorflow-microlite.a

C_OBJS  +=  $(ROOT_DIR)/testxa_af_hostless/test/plugins/cadence/tflm_common/tflm-inference-api.o	\
            $(ROOT_DIR)/testxa_af_hostless/test/plugins/cadence/tflm_common/xa-tflm-inference-api.o	\
            $(ROOT_DIR)/testxa_af_hostless/test/plugins/cadence/tflm_microspeech/microspeech_model_data.o	\
            $(ROOT_DIR)/testxa_af_hostless/test/plugins/cadence/tflm_microspeech/microspeech-frontend-wrapper-api.o	\
            $(ROOT_DIR)/testxa_af_hostless/test/plugins/cadence/tflm_microspeech/microspeech-inference-wrapper-api.o	\
            $(ROOT_DIR)/testxa_af_hostless/test/plugins/cadence/tflm_microspeech/xa-microspeech-frontend.o  \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/fft.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/fft_util.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/filterbank.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/filterbank_util.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/frontend.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/frontend_util.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/kiss_fft_int16.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/log_lut.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/log_scale.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/log_scale_util.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/noise_reduction.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/noise_reduction_util.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/pcan_gain_control.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/pcan_gain_control_util.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/window.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/experimental/microfrontend/lib/window_util.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/examples/micro_speech/audio_provider.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/examples/micro_speech/command_responder.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/examples/micro_speech/feature_provider.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/examples/micro_speech/recognize_commands.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/examples/micro_speech/micro_features/micro_features_generator.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/examples/micro_speech/micro_features/micro_model_settings.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/micro_error_reporter.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/tools/make/downloads/kissfft/kiss_fft.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/tools/make/downloads/kissfft/tools/kfc.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/tools/make/downloads/kissfft/tools/kiss_fastfir.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/tools/make/downloads/kissfft/tools/kiss_fftnd.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/tools/make/downloads/kissfft/tools/kiss_fftndr.o \
            $(SRC_DIR)/tflite-micro/tensorflow/lite/micro/tools/make/downloads/kissfft/tools/kiss_fftr.o \
            $(TFLM_LIB)

.DEFAULT_GOAL := tflm_all

tflm_all: $(SRC_DIR)/tflite-micro all

%.o: %.cpp $(TFLM_LIB)
	$(CXX) $(CFLAGS) $(CXX_FLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.cc $(TFLM_LIB)
	$(CXX) $(CFLAGS) $(CXX_FLAGS) $(INCLUDES) -c -o $@ $<

# Always rebuild xa-factory.c affected by defined macro symbols
$(ROOT_DIR)/testxa_af_hostless/test/plugins/xa-factory.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $(ROOT_DIR)/testxa_af_hostless/test/plugins/xa-factory.c

$(TFLM_LIB): $(SRC_DIR)/tflite-micro
	@python3 --version || (echo "Error: Building TFLM requires Python 3.x!" && exit 1)
	@python3 -c "import PIL" || (echo "Error: Building TFLM requires Python Imaging Library! " \
        "Please, use the system package manager to install the missing library (e.g. 'apt install python3-pil' on Ubuntu)." && exit 1)
	cd $(SRC_DIR)/tflite-micro && make -f tensorflow/lite/micro/tools/make/Makefile TARGET=xtensa TARGET_ARCH=hifi4 OPTIMIZED_KERNEL_DIR=$(KERNELS) XTENSA_USE_LIBC=true microlite

$(SRC_DIR)/tflite-micro:
	git clone https://github.com/tensorflow/$(SRC_DIR)/tflite-micro.git $(SRC_DIR)/tflite-micro
	cd $(SRC_DIR)/tflite-micro && git checkout -q bc67066c8374058148e1b140ec211106667f5d56
