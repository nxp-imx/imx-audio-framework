# Process this file with autoconf to produce a configure script.

AC_INIT(imx-dsp, 1.0.15)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_MAINTAINER_MODE

PKG_PROG_PKG_CONFIG

test "x$prefix" = xNONE && prefix=$ac_default_prefix

AC_DEFUN([CHECK_CODEC_DISABLE],
[
    AC_ARG_ENABLE($1,
		[AS_HELP_STRING([--disable-[$1]], [$2])],
		[use_[$1]=$enableval],
		[use_[$1]=yes])
	if test "$use_[$1]" = "yes"; then
	    AM_CONDITIONAL(HAS_LIB_FSL_[$3], true)
		if test "$1" != "uniaudio"; then
		    enabled_[$4]_codec="$enabled_[$4]_codec\n\t\t[$1]"
		fi
	else
	    AM_CONDITIONAL(HAS_LIB_FSL_[$3], false)
		if test "$1" != "uniaudio"; then
		    disabled_[$4]_codec="$disabled_[$4]_codec\n\t\t[$1]"
		fi
	fi
])

AC_DEFUN([CHECK_CODEC_ENABLE],
[
    AC_ARG_ENABLE($1,
		[AS_HELP_STRING([--enable-[$1]], [$2])],
		[use_[$1]=$enableval],
		[use_[$1]="no"])
	if test "$use_[$1]" = "yes"; then
	    AM_CONDITIONAL(FSL_USE_[$3], true)
		echo -e "\nEnable [$3] option"
	else
	    AM_CONDITIONAL(FSL_USE_[$3], false)
	fi
])

# dsp core codecs
CHECK_CODEC_DISABLE([uniaudio], [disable unified audio wrapper], [UNI_AUDIO], [audio])
CHECK_CODEC_DISABLE([dsp], [disable dsp decoder and encoder], [DSP], [audio])

# enable armv8 lib
CHECK_CODEC_ENABLE([armv8], [enable armv8], [ARMV8])

AC_OUTPUT([
Makefile
libfsldspframework.pc
])

echo -e "\tEnabled audio codec:$enabled_audio_codec"
echo -e "\tDisabled audio codec:$disabled_audio_codec"
