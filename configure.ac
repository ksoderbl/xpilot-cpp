AC_INIT([XPilotCPP], [4.5.5], [https://github.com/ksoderbl], [xpilot-cpp], [https://github.com/ksoderbl/xpilot-cpp])
AM_INIT_AUTOMAKE
AC_LANG([C++])
AC_PROG_CXX

AC_MSG_CHECKING([whether $CXX supports C++17])
save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -std=c++17"

AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <optional>]], [[std::optional<int> x;]])],
  [AC_MSG_RESULT([yes])],
  [AC_MSG_ERROR([*** A C++17 compiler is required ***])]
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


