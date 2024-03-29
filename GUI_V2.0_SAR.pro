QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    conf/QRibbon/QRibbon.h \
    core/MainWindow.h \
    core/dataEvalPage.h \
    core/datasetsWindow/chart.h \
    core/datasetsWindow/datasetDock.h \
    core/modelCAMPage.h \
    core/modelChoicePage.h \
    core/modelEvalPage.h \
    core/modelTrainPage.h \
    core/modelVisPage.h \
    core/modelsWindow/modelDock.h \
    core/reinforceTrainPage.h \
    core/sensePage.h \
    core/transferTrainPage.h \
    lib/algorithm/evaluationIndex.h \
    lib/algorithm/myStruct.h \
    lib/algorithm/torchServe.h \
    lib/guiLogic/bashTerminal.h \
    lib/guiLogic/customWidget/imagewidget.h \
    lib/guiLogic/datasetInfo.h \
    lib/guiLogic/modelInfo.h \
    lib/guiLogic/testThread.h \
    lib/guiLogic/tinyXml/tinystr.h \
    lib/guiLogic/tinyXml/tinyxml.h \
    lib/guiLogic/tools/convertTools.h \
    lib/guiLogic/tools/searchFolder.h
SOURCES += \
    core/dataEvalPage.cpp \
    core/modelCAMPage.cpp \
    core/modelEvalPage.cpp \
    core/modelTrainPage.cpp \
    core/modelVisPage.cpp \
    core/reinforceTrainPage.cpp \
    core/transferTrainPage.cpp \
    lib/algorithm/evaluationIndex.cpp \
    lib/algorithm/torchServe.cpp \
    lib/guiLogic/customWidget/imagewidget.cpp \
    lib/guiLogic/testThread.cpp \
    main.cpp \
    conf/QRibbon/QRibbon.cpp \
    core/MainWindow.cpp \
    core/datasetsWindow/chart.cpp \
    core/datasetsWindow/datasetDock.cpp \
    core/modelChoicePage.cpp \
    core/modelsWindow/modelDock.cpp \
    core/sensePage.cpp \
    lib/guiLogic/bashTerminal.cpp \
    lib/guiLogic/datasetInfo.cpp \
    lib/guiLogic/modelInfo.cpp \
    lib/guiLogic/tinyXml/tinystr.cpp \
    lib/guiLogic/tinyXml/tinyxml.cpp \
    lib/guiLogic/tinyXml/tinyxmlerror.cpp \
    lib/guiLogic/tinyXml/tinyxmlparser.cpp \
    lib/guiLogic/tools/searchFolder.cpp

FORMS += \
    ./conf/QRibbon/qribbon.ui \
    ./uis/MainWindow.ui

TRANSLATIONS += \
    ./conf/QRibbon_yue_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    ./conf/QRibbon/QRibbon.qrc \
    ./sources/MainWindow_sources.qrc \
    conf/QRibbon/QRibbon.qrc \
    sources/MainWindow_sources.qrc

include("./subGUI/subGUI.pri")


LIBS += /usr/local/lib/libopencv_calib3d.so \
/usr/local/lib/libopencv_core.so \
/usr/local/lib/libopencv_features2d.so \
/usr/local/lib/libopencv_flann.so \
/usr/local/lib/libopencv_highgui.so \
/usr/local/lib/libopencv_imgcodecs.so \
/usr/local/lib/libopencv_imgproc.so \
/usr/local/lib/libopencv_ml.so \
/usr/local/lib/libopencv_objdetect.so \
/usr/local/lib/libopencv_photo.so \
/usr/local/lib/libopencv_shape.so \
/usr/local/lib/libopencv_stitching.so \
/usr/local/lib/libopencv_superres.so \
/usr/local/lib/libopencv_videoio.so \
/usr/local/lib/libopencv_video.so \
/usr/local/lib/libopencv_videostab.so

RC_ICONS = "./sources/LOGO.ico"
