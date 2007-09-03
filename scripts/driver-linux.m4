dnl Set Linux kernel headers path.
dnl ---------------------------------------------------------------------------

AC_DEFUN([PCIMAXFM_PATH_LINUX_HEADERS],
  [
    AC_MSG_CHECKING([for Linux kernel headers])

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
*** Linux kernel headers not found.
*** Set the path manually by using --with-kerneldir=DIR.
      ])
    fi

    AC_ARG_WITH([kerneldir],
      AS_HELP_STRING([--with-kerneldir=DIR], [Linux kernel headers path]),
      [KERNEL_DIR=$withval],
      [KERNEL_DIR=$detecteddir]
    )
    AC_SUBST(KERNEL_DIR)
    AC_MSG_RESULT($KERNEL_DIR)
  ]
)

dnl Set Linux kernel module path.
dnl ---------------------------------------------------------------------------

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

dnl Set number of devices.
dnl ---------------------------------------------------------------------------

AC_DEFUN([PCIMAXFM_WITH_MAX_DEVS],
  [
    min=1
    max=512

    AC_ARG_WITH([max-devs],
      AS_HELP_STRING([--with-max-devs=NUM], [Max number of devices @<:@1@:>@]),
      [
        if test $withval -ge $min -a $withval -le $max; then
          max_devs=$withval
        else
          AC_MSG_ERROR([
*** --with-max-devs requires an integer paramater in the range of $min-$max.
          ])
        fi
      ],
      [max_devs=1]
    )
    AC_DEFINE_UNQUOTED([PCIMAXFM_MAX_DEVS], [$max_devs], [Max number of devices.])
  ]
)

dnl Set major device number.
dnl ---------------------------------------------------------------------------

AC_DEFUN([PCIMAXFM_WITH_MAJOR],
  [
    min=0
    max=4094

    AC_ARG_WITH([major],
      AS_HELP_STRING([--with-major=NUM], [Major device number @<:@0 (dynamic)@:>@]),
      [
        if test $withval -ge $min -a $withval -le $max; then
          major=$withval
        else
          AC_MSG_ERROR([
*** --with-major requires an integer paramater in the range of $min-$max.
*** A value of 0 (default) indicates dynamic allocation.
          ])
        fi
      ],
      [major=0]
    )
    AC_DEFINE_UNQUOTED([PCIMAXFM_MAJOR], [$major], [Major device number.])
  ]
)
