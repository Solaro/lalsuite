include $(top_srcdir)/gnuscripts/lalsuite_header_links.am

pkginclude_HEADERS = \
	Grid.h \
	LALAtomicDatatypes.h \
	LALConstants.h \
	LALDatatypes.h \
	LALDebugLevel.h \
	LALError.h \
	LALGSL.h \
	LALMalloc.h \
	LALSIMD.h \
	LALStatusMacros.h \
	LALStddef.h \
	LALStdio.h \
	LALStdlib.h \
	LALString.h \
	LALVCSInfoType.h \
	LALVersion.h \
	StringInput.h \
	XLALError.h \
	XLALGSL.h \
	$(END_OF_LIST)

noinst_LTLIBRARIES = libstd.la liblalsimd.la

libstd_la_SOURCES = \
	Grid.c \
	LALDebugLevel.c \
	LALError.c \
	LALGSL.c \
	LALMalloc.c \
	LALString.c \
	LALVCSInfoType.c \
	LALVersion.c \
	StringConvert.c \
	StringToken.c \
	XLALError.c \
	$(END_OF_LIST)

libstd_la_LIBADD = liblalsimd.la
liblalsimd_la_SOURCES = LALSIMD.c
liblalsimd_la_CFLAGS = $(AM_CFLAGS) $(LALSIMD_CFLAGS)

noinst_HEADERS = \
	Grid_source.c \
	$(END_OF_LIST)

EXTRA_DIST = \
	LALConfig.dox \
	LALDatatypes.dox \
	LALError.dox \
	LALMalloc.dox \
	LALStatusMacros.dox \
	$(END_OF_LIST)
