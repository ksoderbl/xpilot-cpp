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

dnl C++17 requirement
AC_MSG_CHECKING([for C++17 support])

save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -std=c++17"
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <optional>]], [[std::optional<int> x;]])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_RESULT([no])
   CXXFLAGS="$save_CXXFLAGS"
   AC_MSG_ERROR([A C++17 compiler is required])]
)
CXXFLAGS="$save_CXXFLAGS -std=c++17"

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

AC_SUBST([SDL2_CFLAGS])
AC_SUBST([SDL2_LIBS])

PKG_CHECK_MODULES([SDL2_IMAGE], [SDL2_image >= 2.0.0], [], [])
PKG_CHECK_MODULES([SDL2_TTF],   [SDL2_ttf >= 2.0.0],   [], [])
PKG_CHECK_MODULES([SDL2_MIXER], [SDL2_mixer >= 2.0.0], [], [])

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

AM_CONDITIONAL([COND_SDL_CLIENT], [test "x$have_sdl_client" = xyes])

AC_CONFIG_FILES([
	Makefile
	contrib/Makefile
	doc/Makefile
	lib/Makefile
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


