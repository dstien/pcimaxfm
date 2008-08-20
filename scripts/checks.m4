dnl Check for header file.
dnl ---------------------------------------------------------------------------

AC_DEFUN([PCIMAXFM_CHECK_HEADER],
  [
    AC_CHECK_HEADER([$1], [], AC_MSG_ERROR([
*** $1 is needed by $2.
    ]))
  ]
)

dnl Check for function.
dnl ---------------------------------------------------------------------------

AC_DEFUN([PCIMAXFM_CHECK_FUNC],
  [
    AC_CHECK_FUNC([$1], [], AC_MSG_ERROR([
*** Function $1(...) needed by $2.
    ]))
  ]
)

dnl Set PCI Max device version and associated settings
dnl ---------------------------------------------------------------------------

AC_DEFUN([PCIMAXFM_WITH_VERSION],
  [
    AC_ARG_WITH([version],
      AS_HELP_STRING([--with-version=200X], [Device version 2004/2005/2006/2007 @<:@2007@:>@]),
      [
        if test $withval = "2004" -o $withval = "2005"; then
          dev_ver=$withval
	  tx=1
          inv_stereo=1
          rds=0
          rds_signal=0
        elif test $withval = "2006"; then
          dev_ver=$withval
	  tx=0
          inv_stereo=1
          rds=1
          rds_signal=0
        elif test $withval = "2007"; then
          dev_ver=$withval
	  tx=0
          inv_stereo=0
          rds=1
          rds_signal=1
        else
          AC_MSG_ERROR([
*** Unsupported device version. Value of --with-version must be
*** 2004, 2005, 2006 or 2007 (default).
          ])
        fi
      ],
      [
        dev_ver="2007"
	tx=0
        inv_stereo=0
        rds=1
        rds_signal=1
      ]
    )

    AC_DEFINE_UNQUOTED([PCIMAXFM_DEVICE_VERSION], ["$dev_ver"], [Device version.])
    AC_DEFINE_UNQUOTED([PCIMAXFM_ENABLE_TX_TOGGLE], [$tx], [Enable transmitter power toggle.])
    AC_DEFINE_UNQUOTED([PCIMAXFM_INVERT_STEREO], [$inv_stereo], [Invert stereo encoder state flag.])
    AC_DEFINE_UNQUOTED([PCIMAXFM_ENABLE_RDS], [$rds], [Enable RDS encoder.])
    AC_DEFINE_UNQUOTED([PCIMAXFM_ENABLE_RDS_TOGGLE], [$rds_signal], [Enable RDS signal toggle.])
  ]
)
