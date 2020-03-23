INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD


include(../../libtelnet/qttelnet.pri)

HEADERS += \
    $$PWD/telnetClient.h

SOURCES += \
    $$PWD/telnetClient.cpp
