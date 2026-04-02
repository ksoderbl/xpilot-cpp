add_executable(xpilot-cpp-mapedit
    events.cpp
    expose.cpp
    file.cpp
    forms.cpp
    grow.cpp
    help.cpp
    main.cpp
    round.cpp
    T_Form.cpp
    T_Handler.cpp
    tools.cpp
    T_Popup.cpp
    T_Toolkit.cpp
)

target_include_directories(xpilot-cpp-mapedit
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/generated
)

target_compile_definitions(xpilot-cpp-mapedit
    PRIVATE
        CONF_DATADIR="${XPILOT_CONF_DATADIR}"
)

target_link_libraries(xpilot-cpp-mapedit
    PRIVATE
        xpcommon
        X11::X11
        m
)

install(TARGETS xpilot-cpp-mapedit RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
