file(GLOB XPCLIENT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(xpclient STATIC
    ${XPCLIENT_SOURCES}
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

if(XPILOT_BUILD_X11_CLIENT)
    add_subdirectory(x11)
endif()

if(XPILOT_BUILD_SDL_CLIENT)
    add_subdirectory(sdl)
endif()
