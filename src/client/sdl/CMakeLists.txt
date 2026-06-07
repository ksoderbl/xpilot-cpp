file(GLOB SDL_CLIENT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

if(XPILOT_USE_SDL_GAMELOOP)
    list(REMOVE_ITEM SDL_CLIENT_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/gameloop_x.cpp"
    )
else()
    list(REMOVE_ITEM SDL_CLIENT_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/gameloop.cpp"
    )
endif()

add_executable(xpilot-cpp-client-sdl
    ${SDL_CLIENT_SOURCES}
)

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

install(TARGETS xpilot-cpp-client-sdl
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
