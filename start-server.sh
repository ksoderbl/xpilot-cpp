#!/bin/sh

# Takes map as argument, for example:
# bash start-server.sh lib/maps/j-a-r-n-o.nl/newdarkhellteams.xp 
# Needs to have xpilot-cpp-server installed.

# Build and install xpilot-cpp-server using:
# bash cmake-build.sh
# sudo bash cmake-install.sh

xpilot-cpp-server -fps 12 +reporttometaserver -map "$1"
