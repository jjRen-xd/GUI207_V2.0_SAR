#ifndef DATASETEVALPAGE_H
#define DATASETEVALPAGE_H

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
#include "./core/dataEvalPage.h"
#include "./core/datasetsWindow/chart.h"
#include "./lib/algorithm/evaluationIndex.h"
#include "./lib/guiLogic/testThread.h"
#include "./lib/algorithm/myStruct.h"
#include "sys/time.h"

class DatasetEvalPage:public QObject{
    Q_OBJECT
public:
    DatasetEvalPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo, TorchServe *globalTorchServe);
    ~DatasetEvalPage();
    float rotateIOUcv(cv::RotatedRect rect1,cv::RotatedRect rect2);
    float apCulcu(std::vector<float> precision,std::vector<float> recall);
    void getClassName(std::string dirPath);
    std::map<std::string, QLabel*> uiResult;
    void updateUiResult();
    void plotConMatrix(std::map<std::string,std::map<std::string, float>> conMatrix,std::vector<std::string> matrixType);
    std::string choicedDatasetPATH;
    QString choicedModelName;
    QString choicedModelType;
    std::vector<std::string> classType;
    std::map<std::string, std::map<std::string, float>> conMatrix;
    std::vector<result_> result;
    

public slots:
    void refreshGlobalInfo();
    void testThread();
    void outThread(std::vector<result_> result_out,std::vector<std::string> classType_out,std::map<std::string,std::map<std::string, float>> conMatrix_out);
    void outThreadStop();
    void outThreadError(bool stopped);
    void saveResult();
private:
    // mmrotate
    TestThread *thread = nullptr;
    Ui_MainWindow *ui;
    BashTerminal *terminal;
    DatasetInfo *datasetInfo;
    ModelInfo *modelInfo;
    TorchServe *torchServe;
    EvaluationIndex *evalBack;

    // 时间计算变量
    struct timeval start,end;
    float timer;

    //指标计算用到参数,全局变量




    std::map<std::string,float> resultMean;
    void histogram(std::vector<result_> result,std::map<std::string,float> resultMean,std::vector<std::string> classType);
    void disDegreeChart(QString &classGT, std::vector<float> &degrees, std::map<int, std::string> &classNames);
};

#endif // DATASETEVALPAGE_H
