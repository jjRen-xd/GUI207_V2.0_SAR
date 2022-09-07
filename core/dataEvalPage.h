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


#include "ui_MainWindow.h"

#include "./lib/guiLogic/bashTerminal.h"
#include "./lib/guiLogic/tools/searchFolder.h"

#include "./lib/guiLogic/modelInfo.h"
#include "./lib/guiLogic/datasetInfo.h"

#include "./lib/algorithm/torchServe.h"

#include "./core/modelEvalPage.h"

#include "./core/datasetsWindow/chart.h"

// class NewBashTerminal:public BashTerminal
// {

// public:
//     NewBashTerminal:public (QLineEdit *inWidget, QTextEdit *outWidget);
//     ~NewBashTerminal:public ();


// private:
//     /* data */
// };
class DataEvalPage:public QObject{
    Q_OBJECT
public:
    DataEvalPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo, TorchServe *globalTorchServe);
    ~DataEvalPage();
    void calcuRectangle(cv::Point centerXY, cv::Size wh, float angle, std::vector<cv::Point> &fourPoints);
    float mmdetIOUcalcu(cv::RotatedRect rect1,cv::RotatedRect rect2);
    float rotateIOUcv(cv::RotatedRect rect1,cv::RotatedRect rect2);
    float apCulcu(std::vector<float> precision,std::vector<float> recall);
    void getClassName(std::string dirPath);
    std::map<std::string, QLabel*> uiResult;
    void updateUiResult();
    void plotConMatrix(std::map<std::string,std::map<std::string, float>> conMatrix,std::vector<std::string> matrixType);


public slots:
    void refreshGlobalInfo();
    int importData();
    int testAll();

private:
    struct gt_info
    {
        std::string imgName;
        std::vector<cv::RotatedRect> gtRect;        // 一个图片有多个gt
        std::vector<int> det;       // 每个gt所对应的匹配标志
    };
    struct pre_info
    {
        std::string imgName;
        cv::RotatedRect preRect;
        float score;        // 置信度
        bool operator <(const pre_info &x)const
        {
            return score>x.score; //降序排列
        }
    };
    //混淆矩阵存储格式
    struct gt_info_cm
    {
        cv::RotatedRect gtRect;        // 一个图片有多个gt
        std::string className;
    };

    struct pre_info_cm
    {
        cv::RotatedRect preRect;
        float score;        // 置信度
        std::string className;
    };

    struct result_
    {
        std::string className;
        int gtNUm;
        int detNUm;
        float fp;
        float tp;
        float ap;
        float recall;
        float precision;
        float cfar;
        float score;
    };

    std::vector<result_> result;
    

    void tpfp(std::vector<pre_info> preInfo,std::vector<gt_info> gtInfo,std::vector<float> &tp,std::vector<float> &fp,float iouThresh);

    Ui_MainWindow *ui;
    BashTerminal *terminal;
    DatasetInfo *datasetInfo;
    ModelInfo *modelInfo;
    TorchServe *torchServe;
    SearchFolder *dirTools = new SearchFolder();

    std::string choicedDatasetPATH;
    QString choicedModelName;
    QString choicedModelType;
    QString choicedSamplePATH;


    //数据集所有类别
    std::vector<std::string> classType;
    //指标计算用到参数

    std::map<std::string,float> resultMean;
    std::map<std::string,std::map<std::string, float>> conMatrix;

    void confusionMatrix(std::map<std::string,std::vector<gt_info_cm>> gtInfo,std::map<std::string,std::vector<pre_info_cm>> preInfo,std::vector<std::string> matrixType,std::map<std::string,std::map<std::string, float>> &Matrix,float iouThresh,float scoreThresh);
    void histogram(std::vector<result_> result,std::map<std::string,float> resultMean);
    void disDegreeChart(QString &classGT, std::vector<float> &degrees, std::map<int, std::string> &classNames);
};

#endif // DATAEVALPAGE_H
