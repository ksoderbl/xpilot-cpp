AM_CPPFLAGS = -I$(top_srcdir)/src/common

bin_PROGRAMS = xpilot-cpp-client

xpilot_cpp_client_SOURCES = \
    about.cpp \
    blockbitmaps.cpp \
    blockbitmaps.h \
    caudio.cpp \
    caudio.h \
    client.cpp \
    client.h \
    colors.cpp \
    configure.cpp \
    configure.h \
    connectparam.h \
    datagram.cpp \
    datagram.h \
    dbuff.cpp \
    dbuff.h \
    debugaudio.cpp \
    default.cpp \
    default.h \
    gfx2d.cpp \
    gfx2d.h \
    gfx3d.cpp \
    gfx3d.h \
    guimap.cpp \
    guimap.h \
    guiobjects.cpp \
    guiobjects.h \
    icon.h \
    join.cpp \
    netclient.cpp \
    netclient.h \
    paint.cpp \
    paintdata.cpp \
    paintdata.h \
    paint.h \
    painthud.cpp \
    paintmacros.h \
    paintmap.cpp \
    paintobjects.cpp \
    paintradar.cpp \
    protoclient.h \
    query.cpp \
    record.cpp \
    record.h \
    simbloods.h \
    sim.cpp \
    talk.cpp \
    talk.h \
    talkmacros.cpp \
    textinterface.cpp \
    texture.h \
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
    xpmread.cpp \
    xpmread.h

xpilot_cpp_client_LDADD = -lX11 -lm ../common/libxpcommon.a

## compiled with CC=gcc CFLAGS=-DSOUND=1  ./configure; make clean; make
## needs libaudio-dev
## xpilot_cpp_client_LDADD = -lX11 -lm -laudio ../common/libxpcommon.a

SUBDIRS = items
