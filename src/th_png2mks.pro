TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        base64.cpp \
        main.cpp

HEADERS += \
    base64.h

LIBS += -lpng -std=c++17
