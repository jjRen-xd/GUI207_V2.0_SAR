#ifndef MODELEVALPAGE_H
#define MODELEVALPAGE_H

#include <QObject>
#include <string>
#include <QString>
#include <vector>


#include "ui_MainWindow.h"

#include "./lib/guiLogic/bashTerminal.h"
#include "./lib/guiLogic/tools/searchFolder.h"

#include "./lib/guiLogic/modelInfo.h"
#include "./lib/guiLogic/datasetInfo.h"

#include "./lib/algorithm/torchServe.h"


class ModelEvalPage:public QObject{
    Q_OBJECT
public:
    ModelEvalPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo, TorchServe *globalTorchServe);
    ~ModelEvalPage();


public slots:
    void refreshGlobalInfo();

    int randSample();
    int importSample();

    int testOneSample();


private:
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

};

#endif // MODELEVALPAGE_H
