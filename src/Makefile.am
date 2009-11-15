# $Id$
# $URL$

AM_CPPFLAGS = -I@top_srcdir@/include

LDADD = libMbt.la

bin_PROGRAMS = Mbt Mbtg

noinst_PROGRAMS = convert demo

Mbt_SOURCES = Mbt.cxx

Mbtg_SOURCES = Mbtg.cxx

demo_SOURCES = demo.cxx

convert_SOURCES = convert.cxx

lib_LTLIBRARIES = libMbt.la
libMbt_la_LDFLAGS= -version-info 0:0:0

libMbt_la_SOURCES = MbtAPI.cxx  Pattern.cxx  TagLex.cxx Sentence.cxx  Tagger.cxx
