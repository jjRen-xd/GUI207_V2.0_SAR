#pragma once
#include <QMainWindow>
#include <QProcess>

// 数据记录类
#include "./lib/guiLogic/datasetInfo.h"
#include "./lib/guiLogic/modelInfo.h"

// 主页面类
#include "./core/sensePage.h"
#include "./core/modelChoicePage.h"
#include "./core/modelEvalPage.h"
#include "./core/dataEvalPage.h"

// 悬浮窗部件类
#include "./core/datasetsWindow/datasetDock.h"
#include "./core/modelsWindow/modelDock.h"
#include "./lib/guiLogic/bashTerminal.h"

// 模型部署类
#include "./lib/algorithm/torchServe.h"

// 界面美化类
#include "./conf/QRibbon/QRibbon.h"


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    static const std::string slash="\\";
#else
    static const std::string slash="/";
#endif

namespace Ui{
    class MainWindow; 
};

class MainWindow: public QMainWindow{
	Q_OBJECT

    public:
        MainWindow(QWidget *parent = Q_NULLPTR);
        ~MainWindow();

    public slots:
        void switchPage();      // 页面切换
        void fullScreen();      // 全屏

private:
        Ui::MainWindow *ui; 
        
        // 悬浮窗
        DatasetDock *datasetDock;
        ModelDock *modelDock;
        BashTerminal *terminal;

        // 主页面
        SenseSetPage *senseSetPage;
        ModelChoicePage *modelChoicePage;
        ModelEvalPage *modelEvalPage;
        DataEvalPage *dataEvalPage;

        // 数据记录
        DatasetInfo *globalDatasetInfo;
        ModelInfo *globalModelInfo;

        // 模型部署
        TorchServe *torchServe;

};
