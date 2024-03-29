#ifndef TRANSFERTRAINPAGE_H
#define TRANSFERTRAINPAGE_H


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

class TransferTrainPage:public QObject
{
    Q_OBJECT
public:
    Ui_MainWindow *ui;
    BashTerminal *terminal;
    DatasetInfo *datasetInfo;
    ModelInfo *modelInfo;
    TorchServe *torchServe;
    ModelDock *modelDock;

    TransferTrainPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo,
                    ModelInfo *globalModelInfo, TorchServe *globalTorchServe, ModelDock *modelDock);
    ~TransferTrainPage();
    void execuCmd(int modeltypeId, QString cmd);   // 开放在终端运行命令接口
    void showTrianResult(int modeltypeId);
    void uiInitial(int modeltypeId);

public slots:
    void refreshGlobalInfo();
    void startNormalTrain();
    void startTransferTrain();
    void monitorTrainProcess(int modeltypeId);

private:    
    QString choicedOpticalData="";
    QString choicedSARData="";
    QString choicedPreModel="";
    QString volume="10";
    QString modelType="FEW_SHOT";
    std::vector<QString> times;
    std::vector<QPushButton *> starTrainBts;
    std::vector<QLineEdit *> saveModelNameEdits;
    std::vector<QString> saveModelNames;
    std::vector<QProcess *> processTrain;    // 终端命令行输出
    std::vector<QProgressBar *> trainProgressBars;
    std::vector<QLineEdit *> batchsizeEdits;
    std::vector<QString> batchsizes;
    std::vector<QLineEdit *> epochEdits;
    std::vector<QString> epochs;
    std::vector<QLineEdit *> lrEdits;
    std::vector<QString> lrs;
    std::vector<QLabel *> lossLabel;
    std::vector<QLabel *> accLabel;
    std::vector<QLabel *> apValLabel;
    std::vector<QLabel *> conMatrixLabel;
    QString optivalDataType="BBOX";
    QString sarDataType="BBOX";

    // 为了兼容win与linux双平台
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    QString bashApi = "powershell";            // "Windows" or "Linux"
    #else
    QString bashApi = "bash";            // "Windows" or "Linux"
    #endif

};

#endif // TRANSFERTRAINPAGE_H
