bin_PROGRAMS = xpilot-cpp-client-x11
AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" -I$(top_srcdir)/src/common -I$(top_srcdir)/src/client
xpilot_cpp_client_x11_SOURCES = \
    about.cpp \
    blockbitmaps.cpp \
    blockbitmaps.h \
    colors.cpp \
    configure.cpp \
    configure.h \
    client.cpp \
    client.h \
    datagram.cpp \
    datagram.h \
    dbuff.cpp \
    dbuff.h \
    default.cpp \
    default.h \
    guimap.cpp \
    guimap.h \
    guiobjects.cpp \
    guiobjects.h \
    join.cpp \
    netclient.cpp \
    paint.cpp \
    paint.h \
    paintdata.cpp \
    paintdata.h \
    painthud.cpp \
    paintmap.cpp \
    paintobjects.cpp \
    paintradar.cpp \
    record.cpp \
    record.h \
    talk.cpp \
    talkmacros.cpp \
    textinterface.cpp \
    welcome.cpp \
    widget.cpp \
    widget.h \
    xevent.cpp \
    xevent.h \
    xeventhandlers.cpp \
    xeventhandlers.h \
    xinit.cpp \
    xinit.h \
    xpilot.cpp \
    xpmread.cpp

xpilot_cpp_client_x11_LDADD = $(top_builddir)/src/client/libxpclient.a $(top_builddir)/src/common/libxpcommon.a -lX11 -lm
