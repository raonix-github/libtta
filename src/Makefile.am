EXTRA_DIST =
pkginclude_HEADERS = tta.h
lib_LTLIBRARIES = libtta.la

AM_CPPFLAGS = \
    -include $(top_builddir)/config.h \
    -DSYSCONFDIR=\""$(sysconfdir)"\" \
    -DLIBEXECDIR=\""$(libexecdir)"\" \
    -I${top_srcdir}/src

AM_CFLAGS = ${my_CFLAGS}

libtta_la_SOURCES = \
        tta.c \
        tta.h \
        tta_data.c \
        tta_private.h \
		cc1120.c \
        tta_rf.c \
        tta_rf.h \
        tta_rf_private.h \
        tta_rs485.c \
        tta_rs485.h \
        tta_rs485_private.h \
        tta_tcp.c \
        tta_tcp.h \
        tta_tcp_private.h \
        tta_version.h

libtta_la_LDFLAGS = -no-undefined \
        -version-info $(LIBTTA_LT_VERSION_INFO)

if OS_WIN32
libtta_la_LIBADD = -lwsock32
endif

if OS_QNX
libtta_la_LIBADD = -lsocket
endif

# Header files to install
libttaincludedir = $(includedir)/tta
libttainclude_HEADERS = tta.h tta_version.h tta_rf.h tta_rs485.h tta_tcp.h

DISTCLEANFILES = tta_version.h
EXTRA_DIST += tta_version.h.in
CLEANFILES = *~
