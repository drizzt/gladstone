AC_DEFUN([AM_REFCODE_DOWNLOAD],
  [
    AC_ARG_ENABLE(refcode-download,
      AC_HELP_STRING([--enable-refcode-download], [automatically download reference code]),
      [
        REFCODE_DOWNLOAD=$enableval
      ],
    [REFCODE_DOWNLOAD=no]) dnl Default value
    AM_CONDITIONAL(REFCODE_DOWNLOAD,      test "x$REFCODE_DOWNLOAD" = "xyes")
  ])
