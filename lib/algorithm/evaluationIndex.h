#ifndef EVALUATIONINDEX_H
#define EVALUATIONINDEX_H


#include <QObject>
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


#include "./lib/guiLogic/modelInfo.h"
#include "./lib/guiLogic/datasetInfo.h"
#include "./lib/guiLogic/tools/searchFolder.h" // 防止opencv找不到
#include "./lib/algorithm/torchServe.h"
#include "./lib/algorithm/myStruct.h"
class EvaluationIndex:public QObject{
    Q_OBJECT

public:
    EvaluationIndex(DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo, TorchServe *globalTorchServe);
    ~EvaluationIndex();


    float rotateIOUcv(cv::RotatedRect rect1,cv::RotatedRect rect2);
    float apCulcu(std::vector<float> precision,std::vector<float> recall);
    void confusionMatrix(std::map<std::string,std::vector<gt_info_cm>> gtInfo,std::map<std::string,std::vector<pre_info_cm>> preInfo,std::vector<std::string> matrixType,std::map<std::string,std::map<std::string, float>> &Matrix,float iouThresh,float scoreThresh);
    void tpfp(std::vector<pre_info> preInfo,std::vector<gt_info> gtInfo,std::vector<float> &tp,std::vector<float> &fp,float iouThresh);
    TorchServe *torchServe;
private:
    DatasetInfo *datasetInfo;
    ModelInfo *modelInfo;

};


#endif // EVALUATIONINDEX_H