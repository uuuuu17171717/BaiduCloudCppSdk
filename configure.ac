#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([cppsdk], [1.0.0], [lili19@baidu.com])
AC_CONFIG_SRCDIR([src/bos/client.cpp])
AC_CONFIG_HEADERS([config.h])

AC_ARG_WITH([jsoncpp],
    [AS_HELP_STRING([--with-jsoncpp=DIR],
        [bosfs build with user specified libjsoncpp, default is thirdlib/json])],
    [
        AC_CHECK_FILE($with_jsoncpp,[],[AC_MSG_ERROR("libjsoncpp path $with_jsoncpp not found", 1)])
        CPPFLAGS="${CPPFLAGS} -I$with_jsoncpp/include"
        LDFLAGS="${LDFLAGS} -L$with_jsoncpp/lib"
    ],[
        CPPFLAGS="${CPPFLAGS} -I$(pwd)/thirdlib/json/include"
        LDFLAGS="${LDFLAGS} -L$(pwd)/thirdlib/json/lib"
    ])
AC_CHECK_LIB([jsoncpp], [main])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_PROG_RANLIB

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdint.h stdlib.h string.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MKTIME
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([acl gettimeofday localtime_r])
AM_INIT_AUTOMAKE
AC_CONFIG_FILES([Makefile include/Makefile src/Makefile])
AC_OUTPUT
