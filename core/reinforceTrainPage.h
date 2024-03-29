#ifndef REINFORCETRAINPAGE_H
#define REINFORCETRAINPAGE_H

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

class ReinfoceTrainPage:public QObject
{
    Q_OBJECT
public:
    Ui_MainWindow *ui;
    BashTerminal *terminal;
    DatasetInfo *datasetInfo;
    ModelInfo *modelInfo;
    TorchServe *torchServe;
    ModelDock *modelDock;

    ReinfoceTrainPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo,
                    ModelInfo *globalModelInfo, TorchServe *globalTorchServe, ModelDock *modelDock);
    ~ReinfoceTrainPage();
    void execuCmd(QString cmd);   // 开放在终端运行命令接口
    void refreshDataModelInfo();
    void importModelToSys(QString modelName);
    void showResult(QString resultline);

public slots:
    void startTrain();
    void stopTrain();
    void readLogOutput();      // 读取终端输出并显示

private:
    QProcess *processTrain;    // 终端命令行输出
    std::vector<QLineEdit *> featureWeightEdits;
    QString featureIds = "";
    QString time = "";
    QString batchSize = "";
    QString epoch = "";
    QString lr = "";
    QString dqnBatchSize = "";
    QString dqnEpoch = "";
    QString saveModelName = "";
    QString fusionType="";
    QString modelType="FEA_OPTI";
    QString reinforceDataType="BBOX";
    QString choicedDatasetPATH;
    QString choicedModelPATH;

    // 为了兼容win与linux双平台
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    QString bashApi = "powershell";            // "Windows" or "Linux"
    #else
    QString bashApi = "bash";            // "Windows" or "Linux"
    #endif

};


#endif // REINFORCETRAINPAGE_H
