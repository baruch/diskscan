#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([diskscan], [0.14-dev], [https://github.com/baruch/diskscan/issues])
AC_CONFIG_SRCDIR([diskscan.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
#AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h memory.h stdint.h stdlib.h string.h sys/ioctl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([clock_gettime memset munmap strcasecmp strstr strtol])

# Check the host arch
AC_CANONICAL_HOST


arch_internal="arch-posix.h"
case $host_os in
        linux*) machine=linux; libscsicmd="libscsicmd/libscsicmd.a"; arch_internal="arch-linux.h" ;;
        freebsd*) machine=freebsd; libscsicmd="" ;;
        *) machine=generic; libscsicmd="";;
esac
AC_SUBST(LIBSCSICMD, [$libscsicmd])

AC_CONFIG_LINKS([arch/arch.c:arch/arch-$machine.c
                 include/arch-internal.h:arch/$arch_internal])
AC_CONFIG_FILES([diskscan.do])
AC_OUTPUT
