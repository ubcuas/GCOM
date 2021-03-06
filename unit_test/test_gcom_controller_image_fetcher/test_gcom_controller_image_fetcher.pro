#-------------------------------------------------
#
# Project created by QtCreator 2018-01-20T16:49:35
#
#-------------------------------------------------

QT       += testlib
greaterThan(QT_MAJOR_VERSION, 4): QT += network widgets serialport

TARGET = test_gcom_controller_image_fetcher
CONFIG   += c++14

TEMPLATE = app

INCLUDEPATH += ../../ \
               ../../Mavlink \

DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += \
        test_gcom_controller_image_fetcher.hpp \
        ../../gcom_controller.hpp \
        ../../modules/uas_dcnc/dcnc.hpp \
        ../../modules/uas_message/uas_message.hpp \
        ../../modules/uas_message/uas_message_tcp_framer.hpp \
        ../../modules/uas_message/capabilities_message.hpp \
        ../../modules/uas_message/response_message.hpp \
        ../../modules/uas_message/command_message.hpp \
        ../../modules/uas_message/request_message.hpp \
        ../../modules/uas_message/system_info_message.hpp \
        ../../modules/mavlink_relay/mavlink_relay_tcp.hpp \
        ../../modules/uas_antenna_tracker/antennatracker.hpp \
        ../../modules/uas_message/uas_message_serial_framer.hpp \
        ../../modules/uas_image_fetcher/image_fetcher.hpp \
        ../../modules/uas_message/gps_message.hpp \
        ../../modules/uas_message/imu_message.hpp \
        ../../modules/uas_message/image_message.hpp \
        ../../modules/uas_message/image_untagged_message.hpp \
        ../../modules/uas_message/image_tagged_message.hpp \
        ../../modules/uas_interop_system/InteropObjects/interop_mission.hpp \
        ../../modules/uas_interop_system/InteropObjects/interop_telemetry.hpp \
        ../../modules/uas_interop_system/InteropObjects/moving_obstacle.hpp \
        ../../modules/uas_interop_system/InteropObjects/stationary_obstacle.hpp \
        ../../modules/uas_interop_system/interop.hpp \
        ../../modules/uas_interop_system/InteropObjects/interop_odlc.hpp \
        ../../modules/uas_interop_system/interop_json_interpreter.hpp \
        ../../modules/uas_utility/uas_utility.h \


SOURCES += \
        test_gcom_controller_image_fetcher.cpp \
        ../../gcom_controller.cpp \
        ../../modules/uas_dcnc/dcnc.cpp \
        ../../modules/uas_message/uas_message_tcp_framer.cpp \
        ../../modules/uas_message/capabilities_message.cpp \
        ../../modules/uas_message/response_message.cpp \
        ../../modules/uas_message/command_message.cpp \
        ../../modules/uas_message/request_message.cpp \
        ../../modules/uas_message/system_info_message.cpp \
        ../../modules/mavlink_relay/mavlink_relay_tcp.cpp \
        ../../modules/uas_antenna_tracker/antennatracker.cpp \
        ../../modules/uas_message/uas_message_serial_framer.cpp \
        ../../modules/uas_image_fetcher/image_fetcher.cpp \
        ../../modules/uas_message/gps_message.cpp \
        ../../modules/uas_message/imu_message.cpp \
        ../../modules/uas_message/image_untagged_message.cpp \
        ../../modules/uas_message/image_tagged_message.cpp \
        ../../modules/uas_interop_system/InteropObjects/interop_mission.cpp \
        ../../modules/uas_interop_system/InteropObjects/interop_telemetry.cpp \
        ../../modules/uas_interop_system/InteropObjects/moving_obstacle.cpp \
        ../../modules/uas_interop_system/InteropObjects/stationary_obstacle.cpp \
        ../../modules/uas_interop_system/interop.cpp \
        ../../modules/uas_interop_system/InteropObjects/interop_odlc.cpp \
        ../../modules/uas_interop_system/interop_json_interpreter.cpp \
        ../../modules/uas_utility/uas_utility.cpp \

FORMS += ../../gcomcontroller.ui
