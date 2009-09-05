# $Id$
# $URL$

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src include docs example init

EXTRA_DIST = bootstrap AUTHORS TODO NEWS mbt.pc m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mbt.pc

docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = ChangeLog

$(top_srcdir)/ChangeLog: $(top_srcdir)/NEWS
	svn update; svn2cl --include-rev
