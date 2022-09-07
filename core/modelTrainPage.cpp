#include "modelTrainPage.h"
#include <QMessageBox>
#include <QFileDialog>

ModelTrainPage::ModelTrainPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo):
    ui(main_ui),terminal(bash_terminal),datasetInfo(globalDatasetInfo),modelInfo(globalModelInfo){

//    processTrain = new ModelTrain(ui->textBrowser, NULL, NULL, NULL, ui->trainProgressBar);

    connect(ui->datadirButton, &QPushButton::clicked, this, &ModelTrainPage::chooseDataDir);
    connect(ui->startTrainButton, &QPushButton::clicked, this, &ModelTrainPage::startTrain);
    connect(ui->stopTrainButton, &QPushButton::clicked, this, &ModelTrainPage::stopTrain);
    connect(ui->stopTrainButton, &QPushButton::clicked, this, &ModelTrainPage::stopTrain);

    connect(ui->feaConfusionCheckBox, &QCheckBox::stateChanged, this, &ModelTrainPage::useFeaConfusion);
    connect(ui->feaAutoChooseBox, &QCheckBox::stateChanged, this, &ModelTrainPage::useFeaAutoChoose);


    ui->feaAutoChooseBox->setCheckable(false);
    ui->feaHandChooseBox->setEnabled(false);
}

void ModelTrainPage::chooseDataDir(){
    QString dataPath = QFileDialog::getExistingDirectory(NULL,"请选择待训练数据的根目录","./",QFileDialog::ShowDirsOnly);
    if(dataPath == ""){
        QMessageBox::warning(NULL,"提示","未选择有效数据集根目录!");
        ui->dataDirEdit->setText("");
        return;
    }
    ui->dataDirEdit->setText(dataPath);
}


void ModelTrainPage::useFeaConfusion(){
    if(ui->feaConfusionCheckBox->isChecked()){
        ui->feaAutoChooseBox->setCheckable(true);
        ui->feaHandChooseBox->setEnabled(true);
    }
    else{
        ui->feaAutoChooseBox->setCheckable(false);
        ui->feaHandChooseBox->setEnabled(false);
    }
}

void ModelTrainPage::useFeaAutoChoose(){
    if(ui->feaAutoChooseBox->isChecked()){
        ui->feaHandChooseBox->setEnabled(false);
    }
    else{
        ui->feaHandChooseBox->setEnabled(true);
    }
}

void ModelTrainPage::startTrain(){
    int modelType=ui->modelTypeBox->currentIndex();
    QString dataDir = ui->dataDirEdit->toPlainText();
    if(dataDir==""){
        QMessageBox::warning(NULL, "配置出错", "请指定待训练数据的根目录!");
        return;
    }
    QString cmd;
    if(modelType==0){
        //TODO 此处判断模型类型和数据是否匹配
        QString useFeature = "false";
        if(ui->feaConfusionCheckBox){
            useFeature = "true";
        }
        QString featureIds = "0";
        QString maxEpoch = ui->epochBox->currentText();
        QString batchSize = ui->batchsizeBox->currentText();
        QString cmd="activate jjren && python ../../db/bash/mmdetection/GUI/config_and_train.py"
                " --data_root "+dataDir+" --use_feature "+useFeature+" --feature_ids "+featureIds+
                " --max_epoch "+maxEpoch+" --batch_size "+batchSize;
    }
    else if(modelType==1){
        //TODO 此处判断模型类型和数据是否匹配
        return;
    }
    qDebug() << cmd;
//    processTrain->startTrain(modelType, cmd);
}

void ModelTrainPage::stopTrain(){
//    processTrain->stopTrain();
}
