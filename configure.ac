#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([mahimahi], [0.1], [ravinet@mit.edu])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([delayshell.cc])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# Add picky CXXFLAGS
PICKY_CXXFLAGS="-std=c++0x -pedantic -Wall -Wextra -Weffc++ -Werror"
AC_SUBST([PICKY_CXXFLAGS])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
