AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" -I$(top_srcdir)/src/common -I$(top_srcdir)/src/client

bin_PROGRAMS = xpilot-cpp-replay

xpilot_cpp_replay_SOURCES = \
    buttons.cpp \
    buttons.h \
    xpilot-replay.cpp \
    xpilot-replay.h

xpilot_cpp_replay_LDADD = -lX11 -lm ../common/libxpcommon.a

SUBDIRS = tools
