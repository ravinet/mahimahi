#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([mahimahi], [0.90], [mahimahi@mit.edu])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/frontend/delayshell.cc])
AC_CONFIG_HEADERS([config.h])

# Add picky CXXFLAGS
CXX11_FLAGS="-std=c++11 -pthread"
PICKY_CXXFLAGS="-pedantic -Wall -Wextra -Weffc++ -Werror"
AC_SUBST([CXX11_FLAGS])
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

AC_CHECK_FILE([/usr/lib/apache2], [APACHE_NAME="apache2"],
  [AC_CHECK_FILE([/usr/lib/httpd], [APACHE_NAME="httpd"],
    [AC_MSG_ERROR([apache module directory not found])])])

# Checks for apache configuration modules
AC_CHECK_FILE([/usr/lib/$APACHE_NAME/modules/mod_dir.so], [],
              [AC_MSG_ERROR([apache2 mod_dir.so is required])])

AC_DEFINE_UNQUOTED([MOD_DIR], ["/usr/lib/$APACHE_NAME/modules/mod_dir.so"], [path to apache2 mod_dir])

AC_CHECK_FILE([/usr/lib/$APACHE_NAME/modules/mod_mpm_prefork.so], [],
              [AC_MSG_ERROR([apache2 mod_mpm_prefork.so is required])])

AC_DEFINE_UNQUOTED([MOD_MPM_PREFORK], ["/usr/lib/$APACHE_NAME/modules/mod_mpm_prefork.so"], [path to apache2 mod_mpm_prefork])

AC_CHECK_FILE([/usr/lib/$APACHE_NAME/modules/mod_mime.so], [],
              [AC_MSG_ERROR([apache2 mod_mime.so is required])])

AC_DEFINE_UNQUOTED([MOD_MIME], ["/usr/lib/$APACHE_NAME/modules/mod_mime.so"], [path to apache2 mod_mime])

AC_CHECK_FILE([/usr/lib/$APACHE_NAME/modules/mod_authz_core.so], [],
              [AC_MSG_ERROR([apache2 mod_authz_core.so is required])])

AC_DEFINE_UNQUOTED([MOD_AUTHZ_CORE], ["/usr/lib/$APACHE_NAME/modules/mod_authz_core.so"], [path to apache2 mod_authz_core])

AC_CHECK_FILE([/usr/lib/$APACHE_NAME/modules/mod_cgi.so], [],
              [AC_MSG_ERROR([apache2 mod_cgi.so is required])])

AC_DEFINE_UNQUOTED([MOD_CGI], ["/usr/lib/$APACHE_NAME/modules/mod_cgi.so"], [path to apache2 mod_cgi])

AC_CHECK_FILE([/usr/lib/$APACHE_NAME/modules/mod_rewrite.so], [],
              [AC_MSG_ERROR([apache2 mod_rewrite.so is required])])

AC_DEFINE_UNQUOTED([MOD_REWRITE], ["/usr/lib/$APACHE_NAME/modules/mod_rewrite.so"], [path to apache2 mod_rewrite])

AC_CHECK_FILE([/usr/lib/$APACHE_NAME/modules/mod_env.so], [],
	      [AC_MSG_ERROR([apache2 mod_env.so is required])])

AC_DEFINE_UNQUOTED([MOD_ENV], ["/usr/lib/$APACHE_NAME/modules/mod_env.so"], [path to apache2 mod_env])

AC_CHECK_FILE([/usr/lib/$APACHE_NAME/modules/mod_ssl.so], [],
              [AC_MSG_ERROR([apache2 mod_ssl.so is required])])

AC_DEFINE_UNQUOTED([MOD_SSL], ["/usr/lib/$APACHE_NAME/modules/mod_ssl.so"], [path to apache2 mod_ssl])

# Checks for SSL certificate file
AC_CHECK_FILE([/etc/ssl/certs/ssl-cert-snakeoil.pem], [],
              [AC_MSG_ERROR([ssl certificate file is required])])

AC_DEFINE_UNQUOTED([MOD_SSL_CERTIFICATE_FILE], ["/etc/ssl/certs/ssl-cert-snakeoil.pem"], [path to ssl certificate file])

AC_DEFINE_UNQUOTED([MOD_SSL_KEY], ["/etc/ssl/private/ssl-cert-snakeoil.key"], [path to ssl key])

# Set path to replayshell cgi script
if test "${prefix}" = "NONE"; then
    prefix=$ac_default_prefix
fi
AC_DEFINE_UNQUOTED([REPLAYSERVER], ["${prefix}/bin/nph-replayserver.cgi"], [path to nph-replayserver.cgi])

# Set path to directory with installed executable programs
AC_DEFINE_UNQUOTED([EXEC_DIR], ["${prefix}/bin/"], [path to directory with installed executable programs])

# Checks for libraries.
PKG_CHECK_MODULES([protobuf], [protobuf])
PKG_CHECK_MODULES([libssl], [libcrypto libssl])

# Checks for header files.
AC_HEADER_RESOLV
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h paths.h sys/ioctl.h sys/socket.h unistd.h])

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
PKG_CHECK_MODULES([XCBPRESENT], [xcb-present])
PKG_CHECK_MODULES([PANGOCAIRO], [pangocairo])

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
