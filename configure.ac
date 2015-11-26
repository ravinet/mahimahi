#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([mahimahi], [0.91a], [mahimahi@mit.edu])
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

AC_ARG_VAR([APACHE2], [path to apache2])
AC_PATH_PROGS([APACHE2], [apache2 httpd], [no], [$PATH$PATH_SEPARATOR/sbin$PATH_SEPARATOR/usr/sbin$PATH_SEPARATOR/bin$PATH_SEPARATOR/usr/bin])
if test "$APACHE2" = "no"; then
  AC_MSG_ERROR([apache2 is required])
fi
AC_DEFINE_UNQUOTED([APACHE2], ["$APACHE2"], [path to apache2])

AC_ARG_VAR([DNSMASQ], [path to dnsmasq])
AC_PATH_PROG([DNSMASQ], [dnsmasq], [no], [$PATH$PATH_SEPARATOR/sbin$PATH_SEPARATOR/usr/sbin$PATH_SEPARATOR/bin$PATH_SEPARATOR/usr/bin])
if test "$DNSMASQ" = "no"; then
  AC_MSG_ERROR([dnsmasq is required])
fi
AC_DEFINE_UNQUOTED([DNSMASQ], ["$DNSMASQ"], [path to dnsmasq])

AC_PATH_PROG([PROTOC], [protoc], [])
AS_IF([test x"$PROTOC" = x],
  [AC_MSG_ERROR([cannot find protoc, the Protocol Buffers compiler])])

AC_PATH_PROG([APXS], [apxs], [])
AS_IF([test x"$APXS" = x],
  [AC_MSG_ERROR([cannot find apxs, the Apache extension tool])])

AC_SUBST([APACHE2_MODULE_DIR], [$(apxs -q exp_libexecdir)])
AC_SUBST([APACHE2_INCLUDE], [$(apxs -q exp_includedir)])

# Checks for apache configuration modules

AC_CHECK_FILE([$APACHE2_MODULE_DIR/mod_mpm_prefork.so],
	      [AC_DEFINE_UNQUOTED([MOD_MPM_PREFORK], ["$APACHE2_MODULE_DIR/mod_mpm_prefork.so"], [mod_prefork.so])],
              [AC_MSG_ERROR([apache2 mod_mpm_prefork.so is required])])

AC_CHECK_FILE([$APACHE2_MODULE_DIR/mod_authz_core.so],
	      [AC_DEFINE_UNQUOTED([MOD_AUTHZ_CORE], ["$APACHE2_MODULE_DIR/mod_authz_core.so"], [mod_authz_core.so])],
              [AC_MSG_ERROR([apache2 mod_authz_core.so is required])])

AC_CHECK_FILE([$APACHE2_MODULE_DIR/mod_ssl.so],
	      [AC_DEFINE_UNQUOTED([MOD_SSL], ["$APACHE2_MODULE_DIR/mod_ssl.so"], [mod_ssl.so])],
              [AC_MSG_ERROR([apache2 mod_ssl.so is required])])

# Checks for SSL certificate file
AC_CHECK_FILE([/etc/ssl/certs/ssl-cert-snakeoil.pem], [],
              [AC_MSG_ERROR([ssl certificate file is required])])

AC_DEFINE_UNQUOTED([MOD_SSL_CERTIFICATE_FILE], ["/etc/ssl/certs/ssl-cert-snakeoil.pem"], [path to ssl certificate file])

AC_DEFINE_UNQUOTED([MOD_SSL_KEY], ["/etc/ssl/private/ssl-cert-snakeoil.key"], [path to ssl key])

# Set path to mm-replayserver script
if test "${prefix}" = "NONE"; then
    prefix=$ac_default_prefix
fi
AC_DEFINE_UNQUOTED([REPLAYSERVER], ["${prefix}/bin/mm-replayserver"], [path to mm-replayserver])

AC_DEFINE_UNQUOTED([MOD_DEEPCGI], ["${prefix}/lib/${DEB_HOST_MULTIARCH}/libmod_deepcgi.so"], [path to apache2 mod_deepcgi])

# Checks for libraries.
PKG_CHECK_MODULES([protobuf], [protobuf])
PKG_CHECK_MODULES([libssl], [libcrypto libssl])
PKG_CHECK_MODULES([libapr1], [apr-1])
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
		 src/http/Makefile
		 src/httpserver/Makefile
		 src/frontend/Makefile
		 src/protobufs/Makefile
		 src/tests/Makefile
		 man/Makefile
		 traces/Makefile
		 scripts/Makefile])
AC_OUTPUT
