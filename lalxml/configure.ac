AC_PREREQ([2.63])
AC_INIT([LALXML],[1.1.15.1],[lal-discuss@ligo.org])
AC_CONFIG_HEADERS([src/config.h src/LALXMLConfig.h])
AC_CONFIG_SRCDIR([src/LALXML.c])
AC_CONFIG_AUX_DIR([gnuscripts])
AC_CONFIG_MACRO_DIR([gnuscripts])
AC_CONFIG_FILES([ \
  Makefile \
  debian/Makefile \
  doxygen/Makefile \
  doxygen/doxygen.cfg \
  include/Makefile \
  include/lal/Makefile \
  lalxml.pc \
  lalxml.spec \
  python/Makefile \
  python/lalxml/Makefile \
  python/lalxml/__init__.py \
  src/LALXMLVCSInfo.c \
  src/LALXMLVCSInfo.h \
  src/Makefile \
  swig/Makefile \
  test/Makefile \
])
AM_INIT_AUTOMAKE([1.11 foreign subdir-objects color-tests parallel-tests])

AC_CANONICAL_HOST

LALSUITE_ARG_VAR

LALSUITE_CHECK_GIT_REPO
LALSUITE_DISTCHECK_CONFIGURE_FLAGS

# set library api info
# 1. start with version information of "0:0:0" for each libtool library
# 2. update the version information only immediately before a public
#    release of your software. More frequent updates are unnecessary,
#    and only guarantee that the current interface number gets larger
#    faster.
# 3. if the library source code has changed at all since the last
#    update, then increment revision ("c:r:a" becomes "c:r+1:a").
# 4. if any interfaces have been added, removed, or changed since the
#    last update, increment current, and set revision to 0.
# 5. if any interfaces have been added since the last public release,
#    then increment age.
# 6. if any interfaces have been removed since the last public release,
#    then set age to 0.
AC_SUBST([LIBCURRENT],[1])
AC_SUBST([LIBREVISION],[7])
AC_SUBST([LIBAGE],[0])
AC_SUBST([LIBVERSION],[${LIBCURRENT}:${LIBREVISION}:${LIBAGE}])

# nightly build
LALSUITE_ENABLE_NIGHTLY

# enable SWIG wrapping modules
LALSUITE_ENABLE_SWIG

# enable Doxygen documentation
LALSUITE_ENABLE_DOXYGEN

# define version/configure info
LALSUITE_VERSION_CONFIGURE_INFO

# enable strict gcc flags
LALSUITE_ENABLE_GCC_FLAGS

# workaround broken libtool multilib support on sl6
LALSUITE_MULTILIB_LIBTOOL_HACK

# use silent build rules
AM_SILENT_RULES([yes])

# check for required compilers
LALSUITE_PROG_COMPILERS

# checks for programs
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_AWK
AC_PROG_SED
LALSUITE_CHECK_PYTHON([2.6])

# check for supported mac os x version
LALSUITE_OSX_VERSION_CHECK

# link tests using libtool
LALSUITE_USE_LIBTOOL

# check for header files
AC_HEADER_STDC

# check for required libraries
AC_CHECK_LIB([m],[main],,[AC_MSG_ERROR([could not find the math library])])

# check for libxml2
PKG_CHECK_MODULES([LIBXML2],[libxml-2.0 >= 2.6],[true],[false])
LALSUITE_ADD_FLAGS([C],[${LIBXML2_CFLAGS}],[${LIBXML2_LIBS}])
AC_CHECK_LIB([xml2],[xmlInitParser],,[AC_MSG_ERROR([cannot find the xml2 library])])

# check for libxml2 headers
AC_CHECK_HEADERS([libxml/tree.h],,[AC_MSG_ERROR([could not find the libxml/tree.h header])])

# check for gsl
PKG_CHECK_MODULES([GSL],[gsl],[true],[false])
LALSUITE_ADD_FLAGS([C],[${GSL_CFLAGS}],[${GSL_LIBS}])
AC_CHECK_LIB([gslcblas],[main],,[AC_MSG_ERROR([could not find the gsl library])])
AC_CHECK_LIB([gsl],[gsl_version],,[AC_MSG_ERROR([could not find the gsl library])])
LALSUITE_CHECK_GSL_VERSION([1.13])
LALSUITE_ENABLE_FAST_GSL

# check for gsl headers
AC_CHECK_HEADERS([gsl/gsl_errno.h],,[AC_MSG_ERROR([could not find the gsl/gsl_errno.h header])])

# check for lal libraries and headers
LALSUITE_CHECK_LIB([LAL],[6.13.0],[LALVersion],[lal/LALStdio.h])
LALSUITE_CHECK_LIB([LALSupport],[6.13.0],[XLALCacheImport],[lal/LALCache.h])

# configure SWIG wrapping modules
LALSUITE_USE_SWIG

# configure Doxygen documentation
LALSUITE_USE_DOXYGEN

# add gcc specific flags
LALSUITE_ADD_GCC_FLAGS

# link any executables against LALXML library
AC_SUBST([LDADD], ["\$(top_builddir)/src/liblalxml.la"])

# add extra preprocessor flags
AM_CPPFLAGS="-DPKG_DATA_DIR='\"\$(pkgdatadir)/\"' ${AM_CPPFLAGS}"
AM_CPPFLAGS="-I\$(top_builddir)/include ${AM_CPPFLAGS}"

# force headers to be installed in ${includedir}/lal, ignoring library name
AC_SUBST([pkgincludedir], ["\${includedir}/lal"])

# force all python modules into the exec dir to prevent mixed C and
# pure-python library code from being split across lib and lib64 on
# RedHat-derived systems.
AC_SUBST([pkgpythondir], ["\${pkgpyexecdir}"])

# output
AC_OUTPUT
AC_MSG_NOTICE([
==================================================
LALXML has now been successfully configured:

* Python support is $PYTHON_ENABLE_VAL
* SWIG bindings for Octave are $SWIG_BUILD_OCTAVE_ENABLE_VAL
* SWIG bindings for Python are $SWIG_BUILD_PYTHON_ENABLE_VAL
* Doxygen documentation is $DOXYGEN_ENABLE_VAL

and will be installed under the directory:

${prefix}

Now run 'make' to build LALXML,
and run 'make install' to install LALXML.
==================================================
])
