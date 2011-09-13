# $Id$
# $URL$

AM_CPPFLAGS = -I@top_srcdir@/include

LDADD = libmbt.la

bin_PROGRAMS = mbt mbtg

noinst_PROGRAMS = convert

check_PROGRAMS = simpletest
TESTS = $(check_PROGRAMS) 
TESTS_ENVIRONMENT = topsrcdir=$(top_srcdir)
simpletest_SOURCES = simpletest.cxx


mbt_SOURCES = Mbt.cxx

mbtg_SOURCES = Mbtg.cxx

convert_SOURCES = convert.cxx

lib_LTLIBRARIES = libmbt.la
libmbt_la_LDFLAGS= -version-info 0:0:0

libmbt_la_SOURCES = MbtAPI.cxx Pattern.cxx TagLex.cxx Sentence.cxx \
	Tagger.cxx 
