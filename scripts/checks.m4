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
