add_executable(xpilot-cpp-client-sdl
    DT_drawtext.cpp
    SDL_console.cpp
    SDL_gfxPrimitives.cpp
    console.cpp
    glwidgets.cpp
    images.cpp
    main.cpp
    radar.cpp
    sdlevent.cpp
    sdlgui.cpp
    sdlinit.cpp
    sdlkeys.cpp
    sdlmeta.cpp
    sdlpaint.cpp
    sdlwindow.cpp
    text.cpp
    todo.cpp
)

if(XPILOT_USE_SDL_GAMELOOP)
    target_sources(xpilot-cpp-client-sdl PRIVATE gameloop.cpp)
else()
    target_sources(xpilot-cpp-client-sdl PRIVATE gameloop_x.cpp)
endif()

target_include_directories(xpilot-cpp-client-sdl
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/client
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/generated
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
        ${SDL2_GFX_INCLUDE_DIRS}
)

target_compile_definitions(xpilot-cpp-client-sdl
    PRIVATE
        CONF_DATADIR="${XPILOT_CONF_DATADIR}"
)

target_link_directories(xpilot-cpp-client-sdl
    PRIVATE
        ${SDL2_LIBRARY_DIRS}
        ${SDL2_IMAGE_LIBRARY_DIRS}
        ${SDL2_TTF_LIBRARY_DIRS}
        ${SDL2_MIXER_LIBRARY_DIRS}
        ${SDL2_GFX_LIBRARY_DIRS}
)

target_link_libraries(xpilot-cpp-client-sdl
    PRIVATE
        xpclient
        xpcommon
        ${SDL2_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${SDL2_MIXER_LIBRARIES}
        ${SDL2_GFX_LIBRARIES}
        OpenGL::GL
        OpenGL::GLU
        X11::X11
        ZLIB::ZLIB
        m
)

install(TARGETS xpilot-cpp-client-sdl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
