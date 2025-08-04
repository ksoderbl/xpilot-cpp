bin_PROGRAMS = xpilot-cpp-client-sdl
AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" \
	-I$(top_srcdir)/src/common -I$(top_srcdir)/src/client @SDL_CFLAGS@
xpilot_cpp_client_sdl_SOURCES = \
	asteroid_data.h DT_drawtext.cpp DT_drawtext.h \
	SDL_console.cpp SDL_console.h SDL_gfxPrimitives.cpp SDL_gfxPrimitives.h \
	SDL_gfxPrimitives_font.h console.cpp console.h glwidgets.cpp glwidgets.h \
	images.cpp images.h main.cpp scrap.cpp scrap.h radar.cpp radar.h \
	sdlevent.cpp sdlgui.cpp sdlinit.cpp sdlkeys.cpp sdlkeys.h sdlpaint.cpp \
	sdlpaint.h sdlwindow.cpp sdlwindow.h text.cpp text.h todo.cpp \
	sdlmeta.h sdlmeta.cpp sdlinit.h ../xhacks.h xpclient_sdl.h
xpilot_cpp_client_sdl_LDADD = $(top_builddir)/src/client/libxpclient.a  $(top_builddir)/src/common/libxpcommon.a @GL_LIBS@ @SDL_EXTRA_LIBS@ @X_LIBS@ @X_PRE_LIBS@ @X_EXTENSIONS_LIB@ @X_EXTRA_LIBS@ @SOUND_LIBS@ @W32_LIBS@

# if COND_SDL_GAMELOOP
#   xpilot_cpp_client_sdl_SOURCES += gameloop.cpp
# else
  xpilot_cpp_client_sdl_SOURCES += gameloop_x.cpp
# endif

# if COND_WINDOWS
#   xpilot_cpp_client_sdl_SOURCES += win32hacks.cpp
# else
#   xpilot_cpp_client_sdl_LDADD += -lX11
# endif

EXTRA_DIST = xpilot.rc xpilot_sdl.dsp xpilot_sdl.dsw
