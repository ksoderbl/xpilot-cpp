AC_INIT([XPilotCPP], [4.5.5], [https://github.com/ksoderbl], [xpilot-cpp], [https://github.com/ksoderbl/xpilot-cpp])
AC_CONFIG_SRCDIR([src])
dnl AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([disable])

AC_LANG([C++])
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl --- C++17 requirement ---
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
	src/server/Makefile
	src/replay/Makefile
	src/replay/tools/Makefile
	src/mapedit/Makefile
])
AC_OUTPUT


