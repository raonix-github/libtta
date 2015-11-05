EXTRA_DIST = MIGRATION README.md libtta.spec
CLEANFILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtta.pc
EXTRA_DIST += libtta.pc.in
CLEANFILES += libtta.pc

SUBDIRS = src doc tests
