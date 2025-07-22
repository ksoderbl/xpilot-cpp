AM_CPPFLAGS = -I$(top_srcdir)/src/common -I$(top_srcdir)/src/client

bin_PROGRAMS = xpilot-cpp-replay

xpilot_cpp_replay_SOURCES = buttons.cpp xpilot-replay.cpp

xpilot_cpp_replay_LDADD = -lX11 -lm ../common/libxpcommon.a
