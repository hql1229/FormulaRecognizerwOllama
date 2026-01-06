QT += core testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle
CONFIG += c++11

TEMPLATE = app

SOURCES += \
    configmanager_test.cpp \
    configmanager.cpp

HEADERS += \
    configmanager.h
