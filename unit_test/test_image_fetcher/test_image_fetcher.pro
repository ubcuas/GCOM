#-------------------------------------------------
#
# Project created by QtCreator 2017-12-02T10:42:29
#
#-------------------------------------------------

QT       += testlib
greaterThan(QT_MAJOR_VERSION, 4): QT += network

TARGET = test_image_fetcher
CONFIG   += c++14

TEMPLATE = app

INCLUDEPATH += ../../

DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += \
        test_image_fetcher.hpp \
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
        ../../modules/uas_image_fetcher/image_fetcher.hpp \


    test_image_fetcher.hpp

SOURCES += \
        test_image_fetcher.cpp \
        ../../modules/uas_dcnc/dcnc.cpp \
        ../../modules/uas_message/uas_message_tcp_framer.cpp \
        ../../modules/uas_message/capabilities_message.cpp \
        ../../modules/uas_message/response_message.cpp \
        ../../modules/uas_message/command_message.cpp \
        ../../modules/uas_message/request_message.cpp \
        ../../modules/uas_message/system_info_message.cpp \
        ../../modules/uas_message/image_untagged_message.cpp \
        ../../modules/uas_message/image_tagged_message.cpp \
        ../../modules/uas_image_fetcher/image_fetcher.cpp

# IMAGE_DIR_PATH=\\\"[INSERT PATH]\\\"
# Replace [INSERT PATH] with the absolute path of the directory where the test image directory are located
DEFINES += IMAGE_DIR_PATH=\\\"C:/Users/Igor/Documents/UAS/GCOM/unit_test/test_image_fetcher/image_dir/\\\"
DEFINES += IMAGE_PATH=\\\"C:/Users/Igor/Documents/UAS/GCOM/unit_test/test_image_fetcher/images/\\\"

