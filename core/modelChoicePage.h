#ifndef MODELCHOICEPAGE_H
#define MODELCHOICEPAGE_H

#include <map>
#include <vector>
#include <string>
#include <QObject>
#include <iostream>
#include <QButtonGroup>
#include <QString>
#include "ui_MainWindow.h"
#include "./lib/guiLogic/bashTerminal.h"
#include "./lib/guiLogic/modelInfo.h"
#include "./lib/algorithm/torchServe.h"

class ModelChoicePage:public QObject{
    Q_OBJECT
public:
    ModelChoicePage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, ModelInfo *globalModelInfo,TorchServe *globalTorchServe);
    ~ModelChoicePage();

    QButtonGroup *BtnGroup_typeChoice = new QButtonGroup;
    std::map<std::string, QLineEdit*> attriLabelGroup;
    QString lastSelectType = "";
    QString lastSelectName = "";

public slots:
    void changeType();
    void confirmModel(bool notDialog);
    void saveModelAttri();
    void updateAttriLabel();



private:
    Ui_MainWindow *ui;
    BashTerminal *terminal;
    TorchServe *torchServe;
    ModelInfo *modelInfo;
};

#endif // MODELCHOICEPAGE_H
