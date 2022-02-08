#
# Copyright (c) 2015-2021 Cadence Design Systems, Inc.
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#



QUIET =
MAPFILE  = map_$(CODEC_NAME).txt
LDSCRIPT = ldscript_$(CODEC_NAME).txt
SYMFILE  = symbols_$(CODEC_NAME).txt

ifeq ($(CPU), gcc)
    S = /
    AR = ar
    OBJCOPY = objcopy
    CC = gcc
    CFLAGS += -DCSTUB=1
    CFLAGS += -ffloat-store 
    CFLAGS += -D__CSTUB_HIFI2__ -DHIFI2_CSTUB
	RM = rm -f
    RM_R = rm -rf
    MKPATH = mkdir -p
    CP = cp -f
    INCLUDES += \
    -I$(ROOTDIR)/test/include
else
    AR = xt-ar $(XTCORE)
    OBJCOPY = xt-objcopy $(XTCORE)
    CC = xt-xcc $(XTCORE)
    ISS = xt-run $(XTCORE)
    CONFIGDIR := $(shell $(ISS) --show-config=config)
    include $(CONFIGDIR)/misc/hostenv.mk
    CFLAGS += -Wall 
    CFLAGS += -Werror 
    CFLAGS += -mno-mul16 -mno-mul32 -mno-div32 -fsigned-char -mlongcalls -INLINE:requested 
endif

#CFLAGS += -Wno-unused -DXF_TRACE=1
CFLAGS += -DHIFI_ONLY_XAF

ISR_SAFE_CFLAGS = -mcoproc

OBJDIR = objs$(S)$(CODEC_NAME)
LIBDIR = $(ROOTDIR)$(S)lib

OBJ_LIBO2OBJS  = $(addprefix $(OBJDIR)/,$(LIBO2OBJS))
OBJ_LIBOSOBJS  = $(addprefix $(OBJDIR)/,$(LIBOSOBJS))
OBJ_LIBISROBJS = $(addprefix $(OBJDIR)/,$(LIBISROBJS))
OBJS_HOSTOBJS = $(addprefix $(OBJDIR)/,$(HOSTOBJS))

OBJS_LIST = $(OBJS_HOSTOBJS)

TEMPOBJ = temp.o    

ifeq ($(CPU), gcc)
    LIBOBJ   = $(OBJDIR)/xgcc_$(CODEC_NAME).o
    LIB      = xgcc_$(CODEC_NAME).a
else
    LIBOBJ   = $(OBJDIR)/xa_$(CODEC_NAME).o
    LIB      = xa_$(CODEC_NAME).a
endif

CFLAGS += $(EXTRA_CFLAGS)

ifeq ($(DEBUG),1)
  CFLAGS += -DXF_DEBUG=1
  NOSTRIP = 1
  OPT_O2 = -O0 -g 
  OPT_OS = -O0 -g
else
  OPT_O2 = -O2 
ifeq ($(CPU), gcc)
  OPT_OS = -O2 
else
  OPT_OS = -Os 
endif
endif


all: $(OBJDIR) $(LIB) 
$(CODEC_NAME): $(OBJDIR) $(LIB) 

install: $(OBJDIR) $(LIB)
	@echo "Installing $(LIB)"
	$(QUIET) -$(MKPATH) "$(LIBDIR)"
	$(QUIET) $(CP) $(LIB) "$(LIBDIR)"

$(OBJDIR):
	$(QUIET) -$(MKPATH) $@

ifeq ($(NOSTRIP), 1)
$(LIBOBJ): $(OBJ_LIBO2OBJS) $(OBJ_LIBOSOBJS) $(OBJ_LIBISROBJS) $(OBJS_LIST)
	@echo "Linking Objects"
	$(QUIET) $(CC) -o $@ $^ \
	-Wl,-r,-Map,$(MAPFILE) --no-standard-libraries \
	-Wl,--script,$(LDSCRIPT)
else
$(LIBOBJ): $(OBJ_LIBO2OBJS) $(OBJ_LIBOSOBJS) $(OBJ_LIBISROBJS) $(OBJS_LIST)
	@echo "Linking Objects"
	$(QUIET) $(CC) -o $@ $^ \
	-Wl,-r,-Map,$(MAPFILE) --no-standard-libraries \
	-Wl,--retain-symbols-file,$(SYMFILE) \
	-Wl,--script,$(LDSCRIPT)
	$(QUIET) $(OBJCOPY) --keep-global-symbols=$(SYMFILE) $@ $(TEMPOBJ)
	$(QUIET) $(OBJCOPY) --strip-unneeded $(TEMPOBJ) $@
	$(QUIET) -$(RM) $(TEMPOBJ)
endif 


$(OBJ_PLUGINOBJS) $(OBJ_LIBO2OBJS): $(OBJDIR)/%.o: %.c
	@echo "Compiling $<"
	$(QUIET) $(CC) -o $@ $(OPT_O2) $(CFLAGS) $(ISR_SAFE_CFLAGS) $(INCLUDES) -c $<

$(OBJ_LIBISROBJS): $(OBJDIR)/%.o: %.c
	@echo "Compiling $<"
	$(QUIET) $(CC) -o $@ $(OPT_O2) $(CFLAGS) $(INCLUDES) -c $<
	
$(OBJ_LIBOSOBJS): $(OBJDIR)/%.o: %.c
	@echo "Compiling $<"
	$(QUIET) $(CC) -o $@ $(OPT_OS) $(CFLAGS) $(ISR_SAFE_CFLAGS) $(INCLUDES) -c $<
	
$(OBJS_HOSTOBJS): $(OBJDIR)/%.o: %.c
	@echo "Compiling $<"
	$(QUIET) $(CC) -o $@ $(OPT_O2) $(CFLAGS) $(ISR_SAFE_CFLAGS) $(INCLUDES) -c $<

$(LIB): %.a: $(OBJDIR)/%.o
	@echo "Creating Library $@"
	$(QUIET) $(AR) rc $@ $^

clean:
	-$(RM) xa_$(CODEC_NAME).a xgcc_$(CODEC_NAME).a $(LIBDIR)$(S)xa_$(CODEC_NAME).a $(LIBDIR)$(S)xgcc_$(CODEC_NAME).a $(MAPFILE)
	-$(RM_R) $(OBJDIR) 
