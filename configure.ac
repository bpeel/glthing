AC_INIT([glthing], 0.1)

AC_CONFIG_AUX_DIR([build])
AC_CONFIG_MACRO_DIR([build/autotools])
AC_CONFIG_SRCDIR([src/main.c])
AM_CONFIG_HEADER([config.h])
AC_GNU_SOURCE

AC_PROG_CC
AC_PROG_LN_S

AM_INIT_AUTOMAKE([1.9 foreign no-dist-gzip dist-xz tar-ustar])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

GLTHING_EXTRA_CFLAGS=""
GLTHING_EXTRA_LIBS=""


dnl     ============================================================
dnl     Enable strict compiler flags
dnl     ============================================================

# use strict compiler flags only when building from git; the rules for
# distcheck will take care of turning this on when making a release
m4_define([maintainer_default], [yes])
AC_ARG_ENABLE(
  [maintainer-flags],
  [AC_HELP_STRING([--enable-maintainer-flags=@<:@no/yes/error@:>@],
                  [Use strict compiler flags @<:@default=]maintainer_default[@:>@])],
  [],
  enable_maintainer_flags=maintainer_default
)

MAINTAINER_COMPILER_FLAGS="-Wall -Wcast-align -Wuninitialized
                           -Wno-strict-aliasing -Wempty-body -Wformat
                           -Wformat-security -Winit-self -Wundef
                           -Wdeclaration-after-statement
                           -Wpointer-arith -Wmissing-declarations"

AS_CASE(
  [$enable_maintainer_flags],
  [yes],
  [
    AS_COMPILER_FLAGS([MAINTAINER_CFLAGS], [$MAINTAINER_COMPILER_FLAGS])
  ],
  [no],
  [
  ],
  [error],
  [
    MAINTAINER_COMPILER_FLAGS="$MAINTAINER_COMPILER_FLAGS -Werror"
    AS_COMPILER_FLAGS([MAINTAINER_CFLAGS], [$MAINTAINER_COMPILER_FLAGS])
  ],
  [*],
  [AC_MSG_ERROR([Invalid option for --enable-maintainer-flags])]
)

# strip leading spaces
GLTHING_EXTRA_CFLAGS="$GLTHING_EXTRA_CFLAGS ${MAINTAINER_CFLAGS#*  }"

PKG_CHECK_MODULES(EPOXY, [epoxy])
PKG_CHECK_MODULES(SDL2, [sdl2])
PKG_CHECK_MODULES(GL, [gl])

AC_SUBST(GLTHING_EXTRA_CFLAGS)
AC_SUBST(GLTHING_EXTRA_LIBS)

AC_CONFIG_FILES([
        Makefile
        src/Makefile
])

AC_OUTPUT
