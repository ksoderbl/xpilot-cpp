bin_PROGRAMS = xpilot-cpp-client-x11
AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" -I$(top_srcdir)/src/common -I$(top_srcdir)/src/client
xpilot_cpp_client_x11_SOURCES = \
    about.cpp \
    blockbitmaps.cpp \
    blockbitmaps.h \
    colors.cpp \
    colors.h \
    configure.cpp \
    configure.h \
    dbuff.cpp \
    dbuff.h \
    default.cpp \
    default.h \
    guimap.cpp \
    guiobjects.cpp \
    join.cpp \
    paintdata.cpp \
    paintdata.h \
    painthud.cpp \
    paintmap.cpp \
    paintobjects.cpp \
    paintradar.cpp \
    record.cpp \
    record.h \
    simbloods.h \
    sim.cpp \
    talk.cpp \
    talkmacros.cpp \
    textinterface.cpp \
    welcome.cpp \
    widget.cpp \
    widget.h \
    xclient.cpp \
    xclient.h \
    xevent.cpp \
    xevent.h \
    xeventhandlers.cpp \
    xeventhandlers.h \
    xinit.cpp \
    xinit.h \
    xpaint.cpp \
    xpaint.h \
    xpilot.cpp \
    xpmread.cpp

xpilot_cpp_client_x11_LDADD = $(top_builddir)/src/client/libxpclient.a $(top_builddir)/src/common/libxpcommon.a -lX11 -lm
