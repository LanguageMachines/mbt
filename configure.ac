#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $Id$
# $URL$

AC_PREREQ(2.61)
AC_INIT([mbt],[3.2.5],[timbl@uvt.nl])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([.])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])

if test x"${CXXFLAGS+set}" = xset; then
  # the user set CXXFLAGS; don't override it.
  cxx_flags_were_set=true
else
  cxx_flags_were_set=false
fi

if $cxx_flags_were_set; then
  CXXFLAGS=$CXXFLAGS
fi

# Checks for programs.
AC_PROG_CXX( [g++] )
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# when running tests, use CXX
AC_LANG([C++])


# Checks for header files.
AC_CHECK_HEADERS([])
# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# check for pthreads first otherwise this may fail on OpenMp stuff
ACX_PTHREAD([],[AC_MSG_NOTICE([no pthread support found])])
if test x"$acx_pthread_ok" = xyes; then
   LIBS="$PTHREAD_LIBS $LIBS" \
   CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
fi

PKG_PROG_PKG_CONFIG
# Checks for timbl library.

# inspired by feh-1.3.4/configure.ac.  Tnx Tom Gilbert and feh hackers.

# ugly hack when PKG_CONFIG_PATH isn't defined.
# couldn't get it to work otherwise
if test "x$PKG_CONFIG_PATH" = x; then
     export PKG_CONFIG_PATH=""
fi
AC_ARG_WITH(timbl,
       [  --with-timbl=DIR       use timbl installed in <DIR>; 
               note that you can install timbl in a non-default directory with
               ./configure --prefix=<DIR> in the timbl installation directory],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"])
#  AC_MSG_NOTICE( [pkg-config search path: $PKG_CONFIG_PATH] )
PKG_CHECK_MODULES([timbl], [timbl >= 6.4.0] )
CXXFLAGS="$CXXFLAGS $timbl_CFLAGS"
LIBS="$LIBS $timbl_LIBS"

# Checks for library functions.
AC_CHECK_FUNCS([strdup strerror])

AC_C_CONST
AC_TYPE_SIGNAL

AC_CONFIG_FILES([
  Makefile
  mbt.pc
  src/Makefile
  include/Makefile
  include/mbt/Makefile
  docs/Makefile
  example/Makefile
])

AC_OUTPUT
