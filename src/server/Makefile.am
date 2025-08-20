AM_CPPFLAGS = -DCONF_DATADIR=\"$(pkgdatadir)/\" -I$(top_srcdir)/src/common

bin_PROGRAMS = xpilot-cpp-server

xpilot_cpp_server_SOURCES = \
    alliance.cpp \
    asteroid.cpp \
    asteroid.h \
    cannon.cpp \
    cannon.h \
    cell.cpp \
    cmdline.cpp \
    collision.cpp \
    command.cpp \
    connection.h \
    contact.cpp \
    defaults.h \
    event.cpp \
    fileparser.cpp \
    frame.cpp \
    global.h \
    id.cpp \
    item.cpp \
    laser.cpp \
    map.cpp \
    map.h \
    metaserver.cpp \
    metaserver.h \
    netserver.cpp \
    netserver.h \
    object.cpp \
    object.h \
    option.cpp \
    option.h \
    parser.cpp \
    play.cpp \
    player.cpp \
    proto.h \
    robot.cpp \
    robotdef.cpp \
    robot.h \
    rules.cpp \
    saudio.cpp \
    saudio.h \
    sched.cpp \
    sched.h \
    score.cpp \
    score.h \
    serverconst.h \
    server.cpp \
    server.h \
    ship.cpp \
    shot.cpp \
    showtime.cpp \
    stratbot.cpp \
    tuner.cpp \
    tuner.h \
    update.cpp \
    walls.cpp \
    walls.h \
    wildmap.cpp

xpilot_cpp_server_LDADD = -lm ../common/libxpcommon.a
