INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/jpeg_encode.h \
    $$PWD/opj_encode.h \
    $$PWD/pool_send.h \
    $$PWD/utils.h

SOURCES += \
    $$PWD/jpeg_encode.cpp \
    $$PWD/opj_encode.cpp \
    $$PWD/pool_send.cpp \
    $$PWD/utils.cpp

LIBS += -lpthread -ljpeg -lboost_system -lopenjp2
