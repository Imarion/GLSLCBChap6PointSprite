QT += gui core

CONFIG += c++11

TARGET = PointSprite
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    PointSprite.cpp

HEADERS += \
    PointSprite.h

OTHER_FILES += \
    fshader.txt \
    vshader.txt \
    gshader.txt

RESOURCES += \
    shaders.qrc
