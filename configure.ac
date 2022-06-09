#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([mahimahi], [0.98], [mahimahi@mit.edu])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/frontend/delayshell.cc])
AC_CONFIG_HEADERS([config.h])

LT_INIT
AC_CONFIG_MACRO_DIR([m4])

# Add picky CXXFLAGS
CXX11_FLAGS="-std=c++11 -pthread"
PICKY_CXXFLAGS="-pedantic -Wall -Wextra -Weffc++ -Werror"
AC_SUBST([CXX11_FLAGS])
AC_SUBST([PICKY_CXXFLAGS])

# Checks for programs.
AC_PROG_CXX

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

AC_ARG_VAR([DNSMASQ], [path to dnsmasq])
AC_PATH_PROG([DNSMASQ], [dnsmasq], [no], [$PATH$PATH_SEPARATOR/sbin$PATH_SEPARATOR/usr/sbin$PATH_SEPARATOR/bin$PATH_SEPARATOR/usr/bin])
if test "$DNSMASQ" = "no"; then
  AC_MSG_ERROR([dnsmasq is required])
fi
AC_DEFINE_UNQUOTED([DNSMASQ], ["$DNSMASQ"], [path to dnsmasq])

# Checks for libraries.
PKG_CHECK_MODULES([XCB], [xcb])
PKG_CHECK_MODULES([XCBPRESENT], [xcb-present])
PKG_CHECK_MODULES([PANGOCAIRO], [pangocairo])

# Checks for header files.
AC_HEADER_RESOLV
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h paths.h sys/ioctl.h sys/socket.h unistd.h], [], [AC_MSG_ERROR([Missing required header file.])])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([clock_gettime inet_ntoa memset mkdir setenv socket strerror strtol])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 src/util/Makefile
		 src/packet/Makefile
		 src/graphing/Makefile
		 src/frontend/Makefile
		 src/tests/Makefile
		 man/Makefile
		 traces/Makefile
		 scripts/Makefile])
AC_OUTPUT
