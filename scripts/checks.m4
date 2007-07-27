dnl Set Linux kernel sources path.

AC_DEFUN([PCIMAXFM_PATH_LINUX_SOURCES],
  [
    AC_MSG_CHECKING([for Linux kernel sources])

    defaultdir1="/lib/modules/`uname -r`/build"
    defaultdir2="/usr/src/linux"
    defaultdir3="$defaultdir2-`uname -r`"

    if test -d $defaultdir1 -o -L $defaultdir1; then
      detecteddir=$defaultdir1
    elif test -d $defaultdir2 -o -L $defaultdir2; then
      detecteddir=$defaultdir2
    elif test -d $defaultdir3 -o -L $defaultdir3; then
      detecteddir=$defaultdir3
    else
      AC_MSG_ERROR([
*** Linux kernel sources not found.
*** Set the path manually by using --with-kerneldir=DIR.
      ])
    fi

    AC_ARG_WITH([kerneldir],
      AS_HELP_STRING([--with-kerneldir=DIR], [Linux kernel sources path]),
      [KERNEL_DIR=$withval],
      [KERNEL_DIR=$detecteddir]
    )
    AC_SUBST(KERNEL_DIR)
    AC_MSG_RESULT($KERNEL_DIR)
  ]
)

dnl Set Linux kernel module path.

AC_DEFUN([PCIMAXFM_PATH_LINUX_MODULE],
  [
    AC_MSG_CHECKING([for Linux module directory])

    defaultdir="/lib/modules/`uname -r`"
    if test ! -d $defaultdir -a ! -L $defaultdir; then
      AC_MSG_ERROR([
*** Linux kernel module install directory not found.
*** Set the path manually by using --with-moduledir=DIR.
      ])
    fi

    AC_ARG_WITH([moduledir],
      AS_HELP_STRING([--with-moduledir=DIR], [Linux modules install path]),
      [MODULE_DIR=$withval],
      [MODULE_DIR="$defaultdir/extra"]
    )
    AC_SUBST(MODULE_DIR)
    AC_MSG_RESULT($MODULE_DIR)
  ]
)
