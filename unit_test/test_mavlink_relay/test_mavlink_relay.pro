#-------------------------------------------------
#
# Project created by QtCreator 2018-04-10T11:55:14
#
#-------------------------------------------------

QT       += testlib
greaterThan(QT_MAJOR_VERSION, 4): QT += network

TARGET = test_mavlink_relay
CONFIG   += c++14

TEMPLATE = app

INCLUDEPATH += ../../ \
               ../../Mavlink \

DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += \
        test_mavlink_relay.hpp \
        ../../modules/mavlink_relay/mavlink_relay_tcp.hpp \
        ../../modules/uas_interop_system/InteropObjects/interop_mission.hpp \
        ../../modules/uas_interop_system/InteropObjects/interop_telemetry.hpp \
        ../../modules/uas_interop_system/InteropObjects/moving_obstacle.hpp \
        ../../modules/uas_interop_system/InteropObjects/stationary_obstacle.hpp \
        ../../modules/uas_interop_system/interop.hpp \
        ../../modules/uas_interop_system/InteropObjects/interop_odlc.hpp \
        ../../modules/uas_interop_system/interop_json_interpreter.hpp \
        ../../modules/uas_utility/uas_utility.h \


SOURCES += \
        test_mavlink_relay.cpp \
        ../../modules/mavlink_relay/mavlink_relay_tcp.cpp \
        ../../modules/uas_interop_system/InteropObjects/interop_mission.cpp \
        ../../modules/uas_interop_system/InteropObjects/interop_telemetry.cpp \
        ../../modules/uas_interop_system/InteropObjects/moving_obstacle.cpp \
        ../../modules/uas_interop_system/InteropObjects/stationary_obstacle.cpp \
        ../../modules/uas_interop_system/interop.cpp \
        ../../modules/uas_interop_system/InteropObjects/interop_odlc.cpp \
        ../../modules/uas_interop_system/interop_json_interpreter.cpp \
        ../../modules/uas_utility/uas_utility.cpp \
