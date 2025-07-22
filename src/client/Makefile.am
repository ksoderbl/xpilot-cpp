AM_CPPFLAGS = -I$(top_srcdir)/src/common

bin_PROGRAMS = xpilot-cpp-client

xpilot_cpp_client_SOURCES = \
    about.cpp \
    blockbitmaps.cpp \
    client.cpp \
    colors.cpp \
    configure.cpp \
    datagram.cpp \
    dbuff.cpp \
    default.cpp \
    gfx2d.cpp \
    gfx3d.cpp \
    guimap.cpp \
    guiobjects.cpp \
    join.cpp \
    ncdaudio.cpp \
    netclient.cpp \
    paint.cpp \
    paintdata.cpp \
    painthud.cpp \
    paintmap.cpp \
    paintobjects.cpp \
    paintradar.cpp \
    query.cpp \
    record.cpp \
    sim.cpp \
    talk.cpp \
    talkmacros.cpp \
    textinterface.cpp \
    texture.cpp \
    welcome.cpp \
    widget.cpp \
    xevent.cpp \
    xeventhandlers.cpp \
    xinit.cpp \
    xpilot.cpp \
    xpmread.cpp

xpilot_cpp_client_LDADD = -lX11 -lm ../common/libxpcommon.a

## compiled with CC=gcc CFLAGS=-DSOUND=1  ./configure; make clean; make
## needs libaudio-dev
## xpilot_cpp_client_LDADD = -lX11 -lm -laudio ../common/libxpcommon.a
