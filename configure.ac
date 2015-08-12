AC_INIT([famas], [0.0.7a], [andreas.wilm@gmail.com])

# The AC_INIT macro can take any source file as an argument. It just
# checks that the file is there, which should, in turn, mean that the
# source directory is there. AC_INIT generates the following m4
# macros, output variables and preprocessor symbols:
AC_SUBST(PACKAGE_NAME)
AC_SUBST(PACKAGE_TARNAME)
AC_SUBST(PACKAGE_VERSION)
AC_SUBST(PACKAGE_STRING)
AC_SUBST(PACKAGE_BUGREPORT)


AC_MSG_NOTICE([Configuring $PACKAGE_NAME (version $PACKAGE_VERSION) for your system...
                                              
  ______  ____    ____    __  ____    ______  
 |   ___||    \  |    \  /  ||    \  |   ___| 
 |   ___||     \ |     \/   ||     \  `-.`-.  
 |___|   |__|\__\|__/\__/|__||__|\__\|______| 

])
#                                              
# http://patorjk.com/software/taag/#p=display&f=Rammstein&t=famas                                              


AC_PREREQ(2.6) # 2.60 needed for AC_USE_SYSTEM_EXTENSIONS
# consider 2.64 which allows to define a URL as well

# The AM_INIT_AUTOMAKE line adds several standard checks. 
# This macro is always needed for automake
# Obsolete: It takes the program name and version number as arguments.
AM_INIT_AUTOMAKE([foreign])


AC_REVISION([m4_esyscmd_s([git describe --always --dirty])])

AC_LANG([C])

# This macro was introduced in Autoconf 2.60. If possible, enable
# extensions to C or Posix on hosts that normally disable the
# extensions.
AC_USE_SYSTEM_EXTENSIONS

# Set the @SET_MAKE@ variable=make if $(MAKE) not set
AC_PROG_MAKE_SET


# AC_PROG_INSTALL will generate an install target so that users may
# just type 'make install' to install the software.
AC_PROG_INSTALL
AC_PROG_CC
/* make c99 default */
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LN_S
#AC_PROG_RANLIB

# check we can do const and inline
AC_C_CONST
AC_C_INLINE

# check for standard headers
AC_HEADER_STDC


# debugging from http://www.bioinf.uni-freiburg.de/~mmann/HowTo/automake.html
AC_MSG_CHECKING([whether to build with debug information])
AM_CFLAGS="$AM_CFLAGS -Wall -O3"
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
        [enable debug data generation (def=no)])],
    [debugit="$enableval"],
    [debugit=no])
AC_MSG_RESULT([$debugit])
if test x"$debugit" = x"yes"; then
    AC_DEFINE([DEBUG],[],[Debug Mode])
    AM_CFLAGS="$AM_CFLAGS -g -Wall -Werror -Wno-uninitialized -O0"
fi


#AC_ARG_VAR(ARGTABLE3, [*Absolute* path to precompiled argtable3 source directory])
#if test x"$ARGTABLE3" = x""; then
#   AC_MSG_ERROR([Argtable3 directory not defined. Please use ARGTABLE3=/full/path/to/argtable3/])
#fi
#AC_CHECK_FILE(${ARGTABLE3}/argtable3.c, [], [AC_MSG_ERROR([argtable3.c not found])])
#AC_CHECK_FILE(${ARGTABLE3}/argtable3.h, [], [AC_MSG_ERROR([argtable3.c not found])])
#AM_CFLAGS="$AM_CFLAGS -I${ARGTABLE3}"


AC_SUBST([AM_CFLAGS])

# library checks
#
#AC_CHECK_HEADERS([argtable2.h], [],
#                 AC_MSG_ERROR([Could not find argtable2.h. Try $ ./configure CFLAGS='-Iyour-path-to-argtable2-includes]))
#AC_CHECK_LIB(argtable2, arg_parse, [],
#             AC_MSG_ERROR([Could not find libargtable2. Try $ ./configure LDFLAGS="-Lyour-path-to-argtable2-lib']))
AC_CHECK_HEADERS([zlib.h], [],
                 AC_MSG_ERROR([Could not find zlib.h. Try $ ./configure CFLAGS='-Iyour-path-to-zlib-includes]))
AC_CHECK_LIB(z, gzread, [],
             AC_MSG_ERROR([Could not find libz. Try $ ./configure LDFLAGS="-Lyour-path-to-zlib-lib']))

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT
