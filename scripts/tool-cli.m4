dnl Checks for command line user space tool.
dnl ---------------------------------------------------------------------------

AC_DEFUN([PCIMAXFM_TOOL_CLI_CHECKS],
  [
    name="command line user space tool pcimaxctl"

    PCIMAXFM_CHECK_HEADER([stdio.h], [$name])
    PCIMAXFM_CHECK_FUNC([snprintf], [$name])

    PCIMAXFM_CHECK_HEADER([fcntl.h], [$name])
    PCIMAXFM_CHECK_FUNC([ioctl], [$name])

    PCIMAXFM_CHECK_HEADER([getopt.h], [$name])
    PCIMAXFM_CHECK_FUNC([getopt_long], [$name])
  ]
)
