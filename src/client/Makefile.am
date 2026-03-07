AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" -I$(top_srcdir)/src/common

noinst_LIBRARIES = libxpclient.a

libxpclient_a_SOURCES = \
    caudio.cpp \
    caudio.h \
    client.cpp \
    client.h \
    clientcommand.cpp \
    clientcommand.h \
    configure.h \
    connectparam.h \
    datagram.cpp \
    datagram.h \
    debugaudio.cpp \
    default.cpp \
    event.cpp \
    gfx2d.cpp \
    gfx2d.h \
    guimap.h \
    guiobjects.h \
    icon.h \
    mapdata.cpp \
    messages.cpp \
    messages.h \
    meta.cpp \
    meta.h \
    netclient.cpp \
    netclient.h \
    option.cpp \
    option.h \
    paint.cpp \
    paint.h \
    paintmap.cpp \
    paintobjects.cpp \
    paintradar.h \
    query.cpp \
    talk.h \
    talkmacros.cpp \
    textinterface.cpp \
    usleep.cpp

# From section Conditional subdirectories with AM_CONDITIONAL
# in GNU automake manual.
if COND_SDL_CLIENT
  MAYBE_SDL = sdl
else
  MAYBE_SDL =
endif

SUBDIRS = . items x11 $(MAYBE_SDL)
