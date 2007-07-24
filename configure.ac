AC_INIT([pcimaxfm], [0.0.1], [daniel@stien.org], [pcimaxfm])

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([include/config.h])

AC_PROG_CC

BASE_DIR=$(pwd)
AC_SUBST(BASE_DIR)
KERNEL_DIR="/lib/modules/`uname -r`/build/"
AC_SUBST(KERNEL_DIR)

AC_OUTPUT([
	Makefile
	include/Makefile
	src/Makefile
	src/driver/Makefile
	src/driver/linux/Kbuild
	src/driver/linux/Makefile
])
