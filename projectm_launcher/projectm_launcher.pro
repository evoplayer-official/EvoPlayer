QT += core gui widgets opengl
CONFIG += c++17
TARGET = projectm_launcher
TEMPLATE = app
SOURCES += main.cpp
LIBS += -lGL -lprojectM -lpulse-simple -lpulse
INCLUDEPATH += /usr/include/libprojectM
DESTDIR = $$PWD/../build
