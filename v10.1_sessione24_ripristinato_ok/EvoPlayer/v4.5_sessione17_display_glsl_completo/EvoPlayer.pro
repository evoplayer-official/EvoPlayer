QT += multimedia
QT += core gui widgets opengl
CONFIG += c++17
TARGET = EvoPlayer
TEMPLATE = app

SOURCES += \
    main.cpp \
    ui/main_window.cpp \
    audio/qmmp_bridge.cpp \
    engine/camera_controller.cpp \
    engine/model_loader.cpp \
    modules/transport_module.cpp \
    modules/display_module.cpp

HEADERS += \
    ui/main_window.h \
    audio/qmmp_bridge.h \
    engine/camera_controller.h \
    engine/model_loader.h \
    modules/transport_module.h \
    modules/display_module.h

LIBS += -lGL -lGLEW -lassimp -lqmmp-1 -lfreetype
INCLUDEPATH += /usr/include/qmmp-1 /usr/include/assimp /usr/include/freetype2

DESTDIR = $$PWD/build
OBJECTS_DIR = $$PWD/build/obj
