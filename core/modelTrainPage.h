#ifndef MODELTRAINPAGE_H
#define MODELTRAINPAGE_H

#include <QObject>
#include "qlistwidget.h"
#include "ui_MainWindow.h"
#include "modelTrain.h"
#include "./lib/guiLogic/bashTerminal.h"
#include "./lib/guiLogic/modelInfo.h"
#include "./lib/guiLogic/datasetInfo.h"

class ModelTrainPage:public QObject
{
    Q_OBJECT
public:
    Ui_MainWindow *ui;
    BashTerminal *terminal;
    DatasetInfo *datasetInfo;
    ModelInfo *modelInfo;
    BashTerminal *train_terminal;
    QListWidget* featureListWidget;
    QString featureIds = "";
    QStringList features={"Scatter","Peak","Cfar","Harris","Hog","Canny","Countar","Glcm"};

    ModelTrainPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo);

public slots:
    void startTrain();
    void stopTrain();

signals:

private:
    ModelTrain* processTrain;
//    int trainModelType=0;
    QString dataDir;
//    int batchSize;
//    int maxEpoch;

};

#endif // MODELTRAINPAGE_H
