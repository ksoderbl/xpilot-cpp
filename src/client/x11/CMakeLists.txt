file(GLOB X11_CLIENT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(xpilot-cpp-client-x11
    ${X11_CLIENT_SOURCES}
)

target_include_directories(xpilot-cpp-client-x11
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/client
        ${CMAKE_BINARY_DIR}/generated
)

target_compile_definitions(xpilot-cpp-client-x11
    PRIVATE
        CONF_DATADIR="${XPILOT_CONF_DATADIR}"
)

target_link_libraries(xpilot-cpp-client-x11
    PRIVATE
        xpclient
        xpcommon
        X11::X11
        ZLIB::ZLIB
        m
)

install(TARGETS xpilot-cpp-client-x11
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
