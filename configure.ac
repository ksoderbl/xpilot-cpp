AC_INIT([XPilotCPP], [4.5.5], [https://github.com/ksoderbl], [xpilot-cpp], [https://github.com/ksoderbl/xpilot-cpp])
AC_CONFIG_SRCDIR([src])
dnl AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([disable])

AC_LANG([C++])
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG

dnl Check for functions
AC_CHECK_FUNCS([strlcat strlcpy])

dnl C++20 requirement
AC_MSG_CHECKING([for C++20 support])

save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -std=c++20"
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <optional>]], [[std::optional<int> x;]])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no])
   CXXFLAGS="$save_CXXFLAGS"
   AC_MSG_ERROR([A C++20 compiler is required])]
)
CXXFLAGS="$save_CXXFLAGS -std=c++20"

AC_PROG_RANLIB

# SDL2

# Debian:
# sudo apt install libsdl2-dev pkg-config
# sudo apt install libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
# sudo apt install libsdl2-gfx-dev
# sudo apt install libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev
# sudo apt install libx11-dev libxext-dev libxi-dev libxrandr-dev

# or:

# sudo apt install libsdl2-gfx-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev
# sudo apt install libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev

PKG_CHECK_MODULES([SDL2], [sdl2 >= 2.0.0],
  [],
  [AC_MSG_ERROR([SDL2 not found. Install libsdl2-dev (or equivalent).])]
)

dnl AC_SUBST([GL_LIBS])
dnl AC_SUBST([SDL2_LIBS])
dnl AC_SUBST([SDL2_CFLAGS])
dnl AC_SUBST([SDL_EXTRA_LIBS])
dnl AC_SUBST([X_LIBS])
dnl AC_SUBST([X_PRE_LIBS])
dnl AC_SUBST([X_EXTENSIONS_LIB])
dnl AC_SUBST([X_EXTRA_LIBS])
dnl AC_SUBST([SOUND_LIBS])
dnl AC_SUBST([W32_LIBS])

PKG_CHECK_MODULES([SDL2_IMAGE], [SDL2_image >= 2.0.0], [], [])
PKG_CHECK_MODULES([SDL2_TTF],   [SDL2_ttf >= 2.0.0],   [], [])
PKG_CHECK_MODULES([SDL2_MIXER], [SDL2_mixer >= 2.0.0], [], [])

AC_SUBST([SDL2_IMAGE_CFLAGS])
AC_SUBST([SDL2_IMAGE_LIBS])
AC_SUBST([SDL2_TTF_CFLAGS])
AC_SUBST([SDL2_TTF_LIBS])
AC_SUBST([SDL2_MIXER_CFLAGS])
AC_SUBST([SDL2_MIXER_LIBS])

dnl zlib (needed by mapdata.cpp: gzopen/gzread/gzclose)
PKG_CHECK_MODULES([ZLIB], [zlib], [], [
  AC_MSG_ERROR([zlib not found. Install zlib1g-dev (or equivalent).])
])
AC_SUBST([ZLIB_CFLAGS])
AC_SUBST([ZLIB_LIBS])

dnl OpenGL + GLU (Linux/*nix)
PKG_CHECK_MODULES([GL], [gl], [], [
  AC_MSG_ERROR([OpenGL (gl) not found. Install libgl1-mesa-dev (or equivalent).])
])
PKG_CHECK_MODULES([GLU], [glu], [], [
  AC_MSG_ERROR([GLU (glu) not found. Install libglu1-mesa-dev (or equivalent).])
])

dnl Combine GL + GLU into the legacy variables your Makefiles already expect
GL_CFLAGS="$GL_CFLAGS $GLU_CFLAGS"
GL_LIBS="$GL_LIBS $GLU_LIBS"
AC_SUBST([GL_CFLAGS])
AC_SUBST([GL_LIBS])

dnl X11 (needed by scrap.cpp on Linux/X11)
PKG_CHECK_MODULES([X11], [x11], [], [
  AC_MSG_ERROR([X11 not found. Install libx11-dev (or equivalent).])
])
AC_SUBST([X11_CFLAGS])
AC_SUBST([X11_LIBS])

# Option to enable/disable SDL client
AC_ARG_ENABLE([sdl-client],
  AS_HELP_STRING([--enable-sdl-client],
                 [build SDL client (default: auto)]),
  [enable_sdl_client=$enableval],
  [enable_sdl_client=auto]
)

have_sdl_client=no

if test "x$enable_sdl_client" != xno; then
  PKG_CHECK_MODULES([SDL], [sdl2],
    [have_sdl_client=yes],
    [
      if test "x$enable_sdl_client" = xyes; then
        AC_MSG_ERROR([SDL2 requested but not found])
      fi
    ])
fi

# Option to enable/disable the generic SDL game loop
AC_ARG_ENABLE([sdl-gameloop],
  AS_HELP_STRING([--enable-sdl-gameloop],
                 [build SDL client with generic SDL game loop instead of X11 game loop]),
  [enable_sdl_gameloop=$enableval],
  [enable_sdl_gameloop=no]
)

AM_CONDITIONAL([COND_CLIENT], [test x$need_libxpclient = xyes])
AM_CONDITIONAL([COND_WINDOWS], [test x$windows_target = xyes])
AM_CONDITIONAL([COND_REPLAY], [test x$enable_replay = xyes])
AM_CONDITIONAL([COND_XP_MAPEDIT], [test x$enable_xp_mapedit = xyes])
AM_CONDITIONAL([COND_X11_CLIENT], [test x$enable_x11_client = xyes])
AM_CONDITIONAL([COND_SDL_CLIENT], [test x$enable_sdl_client = xyes])
AM_CONDITIONAL([COND_SDL_GAMELOOP], [test x$enable_sdl_gameloop = xyes])

AC_CONFIG_FILES([
  Makefile
  contrib/Makefile
  doc/Makefile
  lib/Makefile
  lib/fonts/Makefile
  lib/maps/Makefile
  lib/textures/Makefile
  src/Makefile
  src/common/Makefile
  src/client/Makefile
  src/client/items/Makefile
  src/client/x11/Makefile
  src/client/sdl/Makefile
  src/server/Makefile
  src/replay/Makefile
  src/replay/tools/Makefile
  src/mapedit/Makefile
])
AC_OUTPUT


