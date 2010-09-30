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

AC_DEFUN([AM_APPLY_PATCHES],
  [
    AC_ARG_ENABLE(apply-patches,
      AC_HELP_STRING([--enable-apply-patches], [apply experimental optimisation patches]),
      [
        APPLY_PATCHES=$enableval
      ],
    [APPLY_PATCHES=yes]) dnl Default value
    AM_CONDITIONAL(APPLY_PATCHES,      test "x$APPLY_PATCHES" = "xyes")
  ])
