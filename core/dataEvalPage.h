#ifndef DATAEVALPAGE_H
#define DATAEVALPAGE_H

#include <QObject>
#include <string>
#include <QString>
#include <vector>
#include <sys/time.h>

#include "ui_MainWindow.h"

#include "./lib/guiLogic/bashTerminal.h"
#include "./lib/guiLogic/tools/searchFolder.h"

#include "./lib/guiLogic/modelInfo.h"
#include "./lib/guiLogic/datasetInfo.h"

#include "./lib/algorithm/torchServe.h"


class DataEvalPage:public QObject{
    Q_OBJECT
public:
    DataEvalPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo, TorchServe *globalTorchServe);
    ~DataEvalPage();

    void calcuRectangle(cv::Point centerXY, cv::Size wh, float angle, std::vector<cv::Point> &fourPoints);



public slots:
    void refreshGlobalInfo();

    int randSample();
    int importImage();
    int importLabel();

    int testOneSample();


    // 绘图展示相关接口
    void showImg_Ori();
    void showImg_GT(cv::Mat &img);
    void showImg_Pred(cv::Mat &img);

    // 复选框相关接口
    void checkboxResponse();



private:
    Ui_MainWindow *ui;
    BashTerminal *terminal;
    DatasetInfo *datasetInfo;
    ModelInfo *modelInfo;
    TorchServe *torchServe;
    SearchFolder *dirTools = new SearchFolder();

    // 选择的数据集、模型、样本信息
    std::string choicedDatasetPATH = "";
    QString choicedModelName = "";
    QString choicedModelType = "";
    QString choicedSamplePATH = "";

    // 预览图的真实及预测标签信息
    cv::Mat imgSrc;
    std::vector<std::vector<std::double_t>> bboxGT;
    std::vector<std::vector<cv::Point>> points_GT;
    std::vector<std::string> labels_GT;
    std::vector<std::vector<cv::Point>> points_Pred;
    std::vector<std::string> labels_Pred;
    std::vector<float> scores_Pred;

};

#endif // DATAEVALPAGE_H
