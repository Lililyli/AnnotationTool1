QT += core gui widgets xml sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = AnnotationTool
TEMPLATE = app

SOURCES += \
    database.cpp \
    dbviewer.cpp \
    inferwidget.cpp \
    main.cpp \
    mainwindow.cpp \
    annotationitem.cpp \
    imageview.cpp \
    labelmanager.cpp \
    quantizewidget.cpp \
    trainwidget.cpp \

HEADERS += \
    database.h \
    dbviewer.h \
    inferwidget.h \
    mainwindow.h \
    annotationitem.h \
    imageview.h \
    labelmanager.h \
    quantizewidget.h \
    trainwidget.h \

DISTFILES += \
    infer.py \
    quantize.py