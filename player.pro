TEMPLATE = app
TARGET = sonig

QMAKE_MAC_SDK = macosx10.12

QT += network \
      xml \
      multimedia \
      multimediawidgets \
      widgets

HEADERS = \
    player.h \
    playercontrols.h \
    playlistmodel.h \
    receiveosc.h \
    oscpkt.hh

SOURCES = main.cpp \
    player.cpp \
    playercontrols.cpp \
    playlistmodel.cpp \
    receiveosc.cpp


