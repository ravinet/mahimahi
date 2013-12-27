#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([mahimahi], [0.1], [ravinet@mit.edu])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([delayshell.cc])
AC_CONFIG_HEADERS([config.h])

# Add picky CXXFLAGS
CPPFLAGS="-std=c++11 -pthread"
PICKY_CXXFLAGS="-pedantic -Wall -Wextra -Weffc++ -Werror -fno-default-inline"
AC_SUBST([PICKY_CXXFLAGS])

# Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB

AC_ARG_VAR([IPTABLES], [path to iptables])
AC_PATH_PROG([IPTABLES], [iptables], [no], [$PATH$PATH_SEPARATOR/sbin$PATH_SEPARATOR/usr/sbin$PATH_SEPARATOR/bin$PATH_SEPARATOR/usr/bin])
if test "$IPTABLES" = "no"; then
  AC_MSG_ERROR([iptables is required])
fi
AC_DEFINE_UNQUOTED([IPTABLES], ["$IPTABLES"], [path to iptables])

AC_ARG_VAR([IP], [path to ip])
AC_PATH_PROG([IP], [ip], [no], [$PATH$PATH_SEPARATOR/sbin$PATH_SEPARATOR/usr/sbin$PATH_SEPARATOR/bin$PATH_SEPARATOR/usr/bin])
if test "$IP" = "no"; then
  AC_MSG_ERROR([ip is required])
fi
AC_DEFINE_UNQUOTED([IP], ["$IP"], [path to ip])

AC_ARG_VAR([APACHE2], [path to apache2])
AC_PATH_PROG([APACHE2], [apache2], [no], [$PATH$PATH_SEPARATOR/sbin$PATH_SEPARATOR/usr/sbin$PATH_SEPARATOR/bin$PATH_SEPARATOR/usr/bin])
if test "$APACHE2" = "no"; then
  AC_MSG_ERROR([apache2 is required])
fi
AC_DEFINE_UNQUOTED([APACHE2], ["$APACHE2"], [path to apache2])

AC_PATH_PROG([PROTOC], [protoc], [])
AS_IF([test x"$PROTOC" = x],
  [AC_MSG_ERROR([cannot find protoc, the Protocol Buffers compiler])])

# Checks for libraries.
PKG_CHECK_MODULES([protobuf], [protobuf])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile protobufs/Makefile])
AC_OUTPUT
