EXTRA_DIST = README

noinst_PROGRAMS = \
	unit_test_master \
	unit_test_slave \
	unit_test_rf \
	version

common_ldflags = \
	$(top_builddir)/src/libtta.la

# my_CFLAGS=--static

unit_test_master_SOURCES = unit_test_master.c unit_test.h
unit_test_master_LDADD = $(common_ldflags)

unit_test_slave_SOURCES = unit_test_slave.c unit_test.h
unit_test_slave_LDADD = $(common_ldflags)

unit_test_rf_SOURCES = unit_test_rf.c
unit_test_rf_LDADD = $(common_ldflags)

version_SOURCES = version.c
version_LDADD = $(common_ldflags)

AM_CPPFLAGS = \
    -include $(top_builddir)/config.h \
    -DSYSCONFDIR=\""$(sysconfdir)"\" \
    -DLIBEXECDIR=\""$(libexecdir)"\" \
    -I${top_srcdir}/src \
    -I${top_builddir}/src

AM_CFLAGS = ${my_CFLAGS}

CLEANFILES = *~
