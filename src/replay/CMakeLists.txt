add_executable(xpilot-cpp-replay
    buttons.cpp
    xpilot-replay.cpp
)

target_include_directories(xpilot-cpp-replay
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_SOURCE_DIR}/src/client
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/generated
)

target_compile_definitions(xpilot-cpp-replay
    PRIVATE
        CONF_DATADIR="${XPILOT_CONF_DATADIR}"
)

target_link_libraries(xpilot-cpp-replay
    PRIVATE
        xpcommon
        X11::X11
        m
)

install(TARGETS xpilot-cpp-replay RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
