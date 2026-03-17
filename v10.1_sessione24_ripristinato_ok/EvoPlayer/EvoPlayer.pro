QT += multimedia
QT += core gui widgets opengl
CONFIG += c++17
TARGET = EvoPlayer
TEMPLATE = app

SOURCES += \
    main.cpp \
    ui/main_window.cpp \
    ui/eq_window.cpp \
    audio/qmmp_bridge.cpp \
    engine/camera_controller.cpp \
    engine/model_loader.cpp \
    modules/transport_module.cpp \
    modules/vumeter_module.cpp \
    modules/display_module.cpp \
    modules/knob_module.cpp \
    modules/power_button.cpp \
    modules/skin_manager.cpp \
    modules/eq_slider.cpp \
    modules/eq_vumeter.cpp \
    ui/library_window.cpp \
    modules/library_screen.cpp \
    modules/visualizer_screen.cpp \
    modules/video_screen.cpp

HEADERS += \
    ui/main_window.h \
    ui/eq_window.h \
    audio/qmmp_bridge.h \
    engine/camera_controller.h \
    engine/model_loader.h \
    modules/transport_module.h \
    modules/vumeter_module.h \
    modules/display_module.h \
    modules/knob_module.h \
    modules/power_button.h \
    modules/skin_manager.h \
    modules/eq_slider.h \
    modules/eq_vumeter.h \
    ui/library_window.h \
    modules/library_screen.h \
    modules/visualizer_screen.h \
    modules/video_screen.h

LIBS += -lGL -lGLEW -lassimp -lqmmp-1 -lfreetype \
        -lvlc \
        -lprojectM
INCLUDEPATH += /usr/include/qmmp-1 /usr/include/assimp /usr/include/freetype2 \
             /usr/include/libprojectM

DESTDIR = $$PWD/build
OBJECTS_DIR = $$PWD/build/obj
