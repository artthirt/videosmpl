TEMPLATE = app
CONFIG += c++11
CONFIG -= app_bundle
CONFIG -= qt

CONFIG += console

DEFINES += _CV _JPEG

OPENCV_DIR = d:\\devs\\opencv\\build\\

INCLUDEPATH += /usr/include/opencv4/ $$OPENCV_DIR/include

SOURCES += main.cpp \
    videov4l2.cpp

win32{
    LIBS += -L$$OPENCV_DIR/x64/vc15/bin

    CONFIG(debug, debug|release){
        LIBS += -lopencv_world451d
    }else{
        LIBS += -lopencv_world451
    }

}else{
    LIBS += -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lv4l2
}

include(net/net.pri)
include(utils/utils.pri)

HEADERS += \
    videov4l2.h
