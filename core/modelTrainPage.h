#ifndef MODELTRAINPAGE_H
#define MODELTRAINPAGE_H

#include <QObject>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include "ui_MainWindow.h"
#include "./lib/guiLogic/bashTerminal.h"
#include "./lib/guiLogic/modelInfo.h"
#include "./lib/guiLogic/datasetInfo.h"
#include "./lib/algorithm/torchServe.h"
#include "./core/modelsWindow/modelDock.h"

class ModelTrainPage:public QObject
{
    Q_OBJECT
public:
    Ui_MainWindow *ui;
    BashTerminal *terminal;
    DatasetInfo *datasetInfo;
    ModelInfo *modelInfo;
    TorchServe *torchServe;
    ModelDock *modelDock;

    ModelTrainPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo,
                    ModelInfo *globalModelInfo, TorchServe *globalTorchServe, ModelDock *modelDock);
    ~ModelTrainPage();
    void execuCmd(QString cmd);   // 开放在终端运行命令接口
    void uiInitial();
    void refreshGlobalInfo();
    
public slots:
    void startTrain();
    void stopTrain();
    void readLogOutput();      // 读取终端输出并显示
    void showTrianResult();
    void handFeaAvailable();

private:
    QProcess *processTrain;    // 终端命令行输出
    std::vector<QCheckBox *> featureBoxs;
    QString featureIds = "";
    QString time = "";
    QString batchSize = "";
    QString epoch = "";
    QString lr = "";
    QString saveModelName = "";
    QString fusionType="";
    QString modelType="TRA_DL";
    bool showLog=true;
    QString choicedDatasetPATH="";

    // 为了兼容win与linux双平台
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    QString bashApi = "powershell";            // "Windows" or "Linux"
    #else
    QString bashApi = "bash";            // "Windows" or "Linux"
    #endif

};

#endif // MODELTRAINPAGE_H
