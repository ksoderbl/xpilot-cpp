add_library(xpclient STATIC
    caudio.cpp
    client.cpp
    clientcommand.cpp
    datagram.cpp
    debugaudio.cpp
    default.cpp
    event.cpp
    gfx2d.cpp
    mapdata.cpp
    messages.cpp
    meta.cpp
    netclient.cpp
    option.cpp
    paint.cpp
    paintmap.cpp
    paintobjects.cpp
    query.cpp
    talkmacros.cpp
    textinterface.cpp
    usleep.cpp
)

target_include_directories(xpclient
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_BINARY_DIR}/generated
)

target_compile_definitions(xpclient
    PUBLIC
        CONF_DATADIR="${XPILOT_CONF_DATADIR}"
)

target_link_libraries(xpclient
    PUBLIC
        xpcommon
        ZLIB::ZLIB
)

add_subdirectory(x11)

if(XPILOT_BUILD_SDL_CLIENT)
    add_subdirectory(sdl)
endif()
