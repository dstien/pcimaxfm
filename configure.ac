AC_INIT([pcimaxfm], [0.0.1], [daniel@stien.org], [pcimaxfm])

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([include/config.h])

AC_DEFINE(
  [PACKAGE_AUTHOR],
  ["Daniel Stien"],
  [Define the author of the package.]
)

AC_DEFINE(
  [PACKAGE_BUGREPORT_AUTHOR],
  [PACKAGE_AUTHOR" <"PACKAGE_BUGREPORT">"],
  [Define the author and email address for bug reports.]
)

AC_DEFINE(
  [PACKAGE_COPYRIGHT],
  ["Copyright (C) 2007 "PACKAGE_AUTHOR],
  [Define the copyright string.]
)

AC_DEFINE(
  [PACKAGE_DESCRIPTION],
  ["Driver for PCI MAX 2007+ and compatible FM transmitter cards."],
  [Define the description of the package.]
)

AC_PROG_CC()

PCIMAXFM_PATH_LINUX_SOURCES()
PCIMAXFM_PATH_LINUX_MODULE()
PCIMAXFM_WITH_MAX_DEVS()
PCIMAXFM_WITH_MAJOR()

BASE_DIR=$(pwd)
AC_SUBST(BASE_DIR)

AC_OUTPUT([
	Makefile
	include/Makefile
	scripts/Makefile
	src/Makefile
	src/driver/Makefile
	src/driver/linux/Kbuild
	src/driver/linux/Makefile
	src/tools/Makefile
	src/tools/pcimaxctl/Makefile
])
