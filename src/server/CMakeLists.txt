file(GLOB SERVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(xpilot-cpp-server ${SERVER_SOURCES})

target_include_directories(xpilot-cpp-server
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/generated
)

target_compile_definitions(xpilot-cpp-server
    PRIVATE
        CONF_DATADIR="${XPILOT_CONF_DATADIR}"
)

target_link_libraries(xpilot-cpp-server
    PRIVATE
        xpcommon
        EXPAT::EXPAT
        ZLIB::ZLIB
        m
)

install(TARGETS xpilot-cpp-server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
