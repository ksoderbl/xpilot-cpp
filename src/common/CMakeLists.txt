add_library(xpcommon STATIC
    recordfile.cpp
    checknames.cpp
    net.cpp
    portability.cpp
    randommt.cpp
    shipshape.cpp
    socklib.cpp
    strcasecmp.cpp
    strdup.cpp
    strlcpy.cpp
    xpconfig.cpp
    xperror.cpp
    xpmath.cpp
    xpmemory.cpp
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

set_target_properties(xpcommon PROPERTIES OUTPUT_NAME xpcommon)
