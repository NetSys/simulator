#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([simulator], [1.1], [akshay.narayan@berkeley.edu])
AC_PREREQ([2.69])
AC_CONFIG_SRCDIR([coresim/main.cpp])

AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_FILES([Makefile])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_CHECK_FUNCS([pow sqrt])


AC_OUTPUT

