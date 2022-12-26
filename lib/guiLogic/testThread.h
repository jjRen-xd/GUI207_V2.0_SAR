#ifndef TESTTHREAD_H
#define TESTTHREAD_H

#include <QThread>
#include <QObject>
#include <iostream>
#include <QDebug>
#include <string>
#include <QString>
#include <vector>
#include <math.h>
#include <stdio.h>
#include <map>
#include <time.h>
#include <QBarSet>
#include <QChart>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QChartView>
#include <QValueAxis>
#include <iomanip>
#include <QMessageBox>
#include <QFileDialog>
#include <algorithm>
#include <cstdio>
#include <QMetaType>
#include "./lib/guiLogic/modelInfo.h"
#include "./lib/guiLogic/datasetInfo.h"
#include "./lib/algorithm/torchServe.h"
#include "./lib/algorithm/evaluationIndex.h"
#include "./lib/guiLogic/tools/searchFolder.h"
#include "./lib/algorithm/myStruct.h"
#include "./core/dataEvalPage.h"

#include "./lib/algorithm/maskApi.h"

typedef  std::map<std::string,std::map<std::string, float>>  CMmap;
// 注册结构体
Q_DECLARE_METATYPE(std::vector<struct result_>);
Q_DECLARE_METATYPE(std::vector<std::string>);
Q_DECLARE_METATYPE(CMmap);

class TestThread:public QThread
{
    Q_OBJECT

public:
    TestThread(DatasetInfo *globalDatasetInfo,
    EvaluationIndex *evalBack,
    std::string *choicedDatasetPATH,
    QString *choicedModelName,
    QString *choicedModelType);
    ~TestThread();
    std::string *choicedDatasetPATH;
    QString *choicedModelName;
    QString *choicedModelType;
    QString choicedSamplePATH;
    std::vector<std::string> classType;
    CMmap conMatrix;
    SearchFolder *dirTools = new SearchFolder();
    void run();
    std::vector<result_> result;
    void stop();
    // 判断是否为正框的标志
    bool bboxTag;
    bool errorTag;
    
signals:
    void end(std::vector<result_> result,std::vector<std::string> classType,CMmap conMatrix);
    void errorStop(bool stopTag);
private:
    DatasetInfo *datasetInfo;
    EvaluationIndex *eval;
    TorchServe *torchServe;
    BashTerminal *terminal;
    MaskApi *maskiou;
    volatile bool stopped;
};

#endif
