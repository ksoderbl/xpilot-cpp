add_executable(xpilot-cpp-client-x11
    about.cpp
    bitmaps.cpp
    colors.cpp
    configure.cpp
    dbuff.cpp
    guimap.cpp
    guiobjects.cpp
    join.cpp
    paintdata.cpp
    painthud.cpp
    paintradar.cpp
    record.cpp
    sim.cpp
    talk.cpp
    welcome.cpp
    widget.cpp
    xdefault.cpp
    xevent.cpp
    xeventhandlers.cpp
    xinit.cpp
    xpaint.cpp
    xpilot.cpp
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

install(TARGETS xpilot-cpp-client-x11 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
