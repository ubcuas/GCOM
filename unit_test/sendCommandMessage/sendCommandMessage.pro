#-------------------------------------------------
#
# Project created by QtCreator 2017-11-18T12:28:05
#
#-------------------------------------------------

QT       += testlib
greaterThan(QT_MAJOR_VERSION, 4): QT += network

TARGET = sendCommandMessage
CONFIG   += c++14

TEMPLATE = app

INCLUDEPATH += ../../

DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += \
        sendcommandmessagetest.h \
        ../../modules/uas_dcnc/dcnc.hpp \
        ../../modules/uas_message/uas_message.hpp \
        ../../modules/uas_message/uas_message_tcp_framer.hpp \
        ../../modules/uas_message/capabilities_message.hpp \
        ../../modules/uas_message/response_message.hpp \
        ../../modules/uas_message/command_message.hpp \
        ../../modules/uas_message/request_message.hpp \
        ../../modules/uas_message/system_info_message.hpp \
        ../../modules/uas_message/image_untagged_message.hpp \
        ../../modules/uas_message/image_tagged_message.hpp \

SOURCES += \
        sendcommandmessagetest.cpp \
        ../../modules/uas_dcnc/dcnc.cpp \
        ../../modules/uas_message/uas_message_tcp_framer.cpp \
        ../../modules/uas_message/capabilities_message.cpp \
        ../../modules/uas_message/response_message.cpp \
        ../../modules/uas_message/command_message.cpp \
        ../../modules/uas_message/request_message.cpp \
        ../../modules/uas_message/system_info_message.cpp \
        ../../modules/uas_message/image_untagged_message.cpp \
        ../../modules/uas_message/image_tagged_message.cpp \
