## Process this file with automake to produce Makefile.in

TOPDIR = .

######################### AUDIO #############################
if HAS_LIB_FSL_DSP


if HAS_LIB_FSL_UNI_AUDIO
if FSL_USE_ARMV8
libuniaaudioinclude =

fsldspwraplib = \
	$(TOPDIR)/release/wrapper/lib_dsp_wrap_arm_elinux.so

fsldspvoicelib = \
	$(TOPDIR)/release/lib/lib_dsp_voice_process_dummy.so

fsldspfirmware = \
	$(TOPDIR)/release/hifi4_imx8mp.bin \
	$(TOPDIR)/release/hifi4_imx8qmqxp.bin \
	$(TOPDIR)/release/hifi4_imx8ulp.bin \
	$(TOPDIR)/release/hifi4_tflm_imx8ulp.bin

endif
endif

endif #end of HAS_LIB_FSL_DSP

pcaudiofile = libfsldspframework.pc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcaudiofile)

libfslaudiocodecincludedir = $(includedir)/imx-mm/audio-codec
libfslaudiocodecinclude_HEADERS = \
	$(libuniaaudioinclude)

libfslaudiowraplibdir = $(libdir)/imx-mm/audio-codec/wrap
libfslaudiowraplib_DATA = \
	$(fsldspwraplib)

libfslaudiovoicelibdir = $(libdir)/imx-mm/audio-codec/dsp
libfslaudiovoicelib_DATA = \
	$(fsldspvoicelib)

libfsldspfirmwaredir = $(datadir)/imx/dsp
libfsldspfirmware_DATA = \
	$(fsldspfirmware)

libfsldspunittestbindir = $(bindir)/DSP
libfsldspunittestbin_DATA = \
	$(TOPDIR)/release/exe/dsp_test.out \
	$(TOPDIR)/release/exe/dsp_rend_test.out \
	$(TOPDIR)/release/exe/dsp_capturer_test.out \
	$(TOPDIR)/release/exe/dsp_tflm_test.out \
	$(TOPDIR)/release/exe/dsp_voiceproc_test.out

libfsldspreleasenotedir =
libfsldspreleasenote_DATA = \
	$(fsldspreleasenote)

EXTRA_DIST = autogen.sh \
	$(libfslaudiovoicelib_DATA) \
	$(libfslaudiowraplib_DATA)  \
	$(libfsldspfirmware_DATA)   \
	$(libfsldspreleasenote_DATA) \
	$(libfsldspunittestbin_DATA)

install-data-hook:
	@for f in $(libfslaudiowraplib_DATA); do                                       \
		base=`basename $$f`;                                                       \
		basepath=$(DESTDIR)$(libfslaudiowraplibdir)/$$base;                        \
		soname=`readelf -d $$basepath | grep soname | sed -e 's/.*\[\(.*\)]/\1/'`; \
		rtname=`echo $$soname | sed -e 's/\(.*\.so\)\..*/\1/'`;                    \
		mv $$basepath $(DESTDIR)$(libfslaudiowraplibdir)/$$soname;                 \
		ln -sf $$soname $(DESTDIR)$(libfslaudiowraplibdir)/$$rtname;               \
	done;                                                                          \
	for f in $(libfsldspaudiovoicelib_DATA); do                                         \
		base=`basename $$f`;                                                       \
		basepath=$(DESTDIR)$(libfslaudiovoicelibdir)/$$base;                         \
	done;                                                                          \
	for f in $(libfsldspfirmware_DATA); do                                         \
		base=`basename $$f`;                                                       \
		basepath=$(DESTDIR)$(libfsldspfirmwaredir)/$$base;                         \
	done;                                                                          \
	for f in $(DESTDIR)$(bindir)/DSP/*; do                                       \
	if [ -f $$f ]; then                                                            \
		chmod +x $$f;                                                          \
	fi;                                                                            \
	done

uninstall-hook:
	@for f in $(libfslaudiowraplib_DATA); do                                       \
		base=`basename $$f`;                                                       \
		rm -f "$(DESTDIR)$(libfslaudiowraplibdir)/$$base"*;                        \
	done;                                                                          \
	for f in $(libfslaudiovoicelib_DATA); do                                       \
		base=`basename $$f`;                                                       \
		rm -f "$(DESTDIR)$(libfslaudiovoicelibdir)/$$base"*;                        \
	done;                                                                          \
	for f in $(libfsldspfirmware_DATA); do                                         \
		base=`basename $$f`;                                                       \
		rm -f "$(DESTDIR)$(libfsldspfirmwaredir)/$$base"*;                         \
	done;                                                                          \
	for f in $(libfsldspunittestbin_DATA); do                                      \
		base=`basename $$f`;                                                       \
		rm -f "$(DESTDIR)$(libfsldspunittestbindir)/$$base"*;                      \
	done

