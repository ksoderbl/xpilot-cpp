bin_PROGRAMS = xpilot-cpp-client-x11
AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" -I$(top_srcdir)/src/common -I$(top_srcdir)/src/client
xpilot_cpp_client_x11_SOURCES = \
    xpilot.cpp

xpilot_cpp_client_x11_LDADD = $(top_builddir)/src/client/libxpclient.a $(top_builddir)/src/common/libxpcommon.a -lX11 -lm
