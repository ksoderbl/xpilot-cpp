file(GLOB XPCOMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(xpcommon STATIC
    ${XPCOMMON_SOURCES}
)

target_include_directories(xpcommon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_BINARY_DIR}/generated
)

target_compile_definitions(xpcommon
    PUBLIC
        CONF_DATADIR="${XPILOT_CONF_DATADIR}"
)

target_link_libraries(xpcommon
    PUBLIC
        m
)

set_target_properties(xpcommon PROPERTIES
    OUTPUT_NAME xpcommon
)
