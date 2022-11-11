#ifndef DATAEVALPAGE_H
#define DATAEVALPAGE_H

#include <QObject>
#include <string>
#include <QString>
#include <vector>
#include <map>
#include <time.h>
#include <QBarSet>
#include <QChart>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QChartView>
#include <QValueAxis>
#include <iomanip>
#include <QEventLoop>
#include "ui_MainWindow.h"

#include "./lib/guiLogic/bashTerminal.h"
#include "./lib/guiLogic/tools/searchFolder.h"
#include "./lib/guiLogic/modelInfo.h"
#include "./lib/guiLogic/datasetInfo.h"
#include "./lib/algorithm/torchServe.h"
#include "./core/modelEvalPage.h"
#include "./core/datasetsWindow/chart.h"
#include "./lib/algorithm/evaluationIndex.h"
#include "./lib/guiLogic/testThread.h"
#include "./lib/algorithm/myStruct.h"


class DataEvalPage:public QObject{
    Q_OBJECT
public:
    DataEvalPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo, TorchServe *globalTorchServe,EvaluationIndex *evalBack);
    ~DataEvalPage();
    float rotateIOUcv(cv::RotatedRect rect1,cv::RotatedRect rect2);
    float apCulcu(std::vector<float> precision,std::vector<float> recall);
    void getClassName(std::string dirPath);
    std::map<std::string, QLabel*> uiResult;
    void updateUiResult();
    void plotConMatrix(std::map<std::string,std::map<std::string, float>> conMatrix,std::vector<std::string> matrixType);
    // std::vector<std::string> classType;
    std::string choicedDatasetPATH;
    QString choicedModelName;
    QString choicedModelType;

public slots:
    void refreshGlobalInfo();
    void ttThread();
    // int testAll();
    void outThread(std::vector<result_> result,std::vector<std::string> classType,std::map<std::string,std::map<std::string, float>> conMatrix);

private:
    // mmrotate
    // EvaluationIndex *evaluate;
    TestThread *thread = nullptr;


    Ui_MainWindow *ui;
    BashTerminal *terminal;
    DatasetInfo *datasetInfo;
    ModelInfo *modelInfo;
    TorchServe *torchServe;
    // SearchFolder *dirTools = new SearchFolder();
    EvaluationIndex *eval;

    // std::string choicedDatasetPATH = "";

    double predTime;
    clock_t start_time,end_time;


    //指标计算用到参数,全局变量
    std::map<std::string,float> resultMean;
    void histogram(std::vector<result_> result,std::map<std::string,float> resultMean,std::vector<std::string> classType);
    void disDegreeChart(QString &classGT, std::vector<float> &degrees, std::map<int, std::string> &classNames);
};

#endif // DATAEVALPAGE_H
