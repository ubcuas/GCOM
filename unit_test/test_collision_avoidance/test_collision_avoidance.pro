#-------------------------------------------------
#
# Project created by QtCreator 2018-01-20T11:46:46
#
#-------------------------------------------------

QT       += testlib
greaterThan(QT_MAJOR_VERSION, 4): QT += network

TARGET = test_collision_avoidance
CONFIG   += c++14

TEMPLATE = app

INCLUDEPATH += ../../

DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS += \
        test_collision_avoidance.hpp \
        ../../modules/uas_collision_avoidance/collision_avoidance.hpp \
        ../../modules/uas_collision_avoidance/mission_planner_command.hpp \
        ../../modules/uas_interop_system/InteropObjects/stationary_obstacle.hpp \
        ../../modules/uas_interop_system/InteropObjects/interop_mission.hpp \

SOURCES += \
        test_collision_avoidance.cpp \
        ../../modules/uas_collision_avoidance/collision_avoidance.cpp \
        ../../modules/uas_interop_system/InteropObjects/stationary_obstacle.cpp \
        ../../modules/uas_interop_system/InteropObjects/interop_mission.cpp \
