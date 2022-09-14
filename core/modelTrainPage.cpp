#include "modelTrainPage.h"
#include <QMessageBox>
#include <QListWidgetItem>
#include <QFileDialog>

ModelTrainPage::ModelTrainPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo):
    ui(main_ui),terminal(bash_terminal),datasetInfo(globalDatasetInfo),modelInfo(globalModelInfo){

    processTrain = new ModelTrain(ui->textBrowser, ui->trainProgressBar);

//    connect(ui->datadirButton,    &QPushButton::clicked, this, &ModelTrainPage::chooseDataDir);
    connect(ui->startTrainButton, &QPushButton::clicked, this, &ModelTrainPage::startTrain);
    connect(ui->stopTrainButton,  &QPushButton::clicked, this, &ModelTrainPage::stopTrain);
}

void ModelTrainPage::startTrain(){
    QString dataDir = "";
    if(dataDir==""){
        QMessageBox::warning(NULL, "配置出错", "请指定待训练数据的根目录!");
        return;
    }
    QString cmd;
    QString useFeature = "false";
//    QString maxEpoch   = ui->epochBox->currentText();
//    QString batchSize  = ui->batchsizeBox->currentText();
//    if(modelType==0){
//        //TODO 此处判断模型类型和数据是否匹配
//        if(ui->feaConfusionCheckBox->isChecked()){
//            useFeature = "true";
//            if(ui->feaAutoChooseBox->isChecked()){
//                return;
//            }
//            else{
//                if(featureIds==""){
//                    QMessageBox::warning(NULL, "特征选择异常", "请选择手动特征或强化学习自选特征!");
//                }else{
//                    cmd="source activate && source deactivate && conda activate 207_base && python ../db/bash/mmdetection/GUI/config_and_train.py --data_root "+
//                            dataDir+" --use_feature "+useFeature+" --feature_ids "+featureIds+
//                            " --max_epoch "+maxEpoch+" --batch_size "+batchSize;
//                }
//            }
//        }
//        else{
//            cmd="source activate && source deactivate && conda activate 207_base && python ../db/bash/mmdetection/GUI/config_and_train.py --data_root "+
//                    dataDir+" --use_feature "+useFeature+" --max_epoch "+maxEpoch+" --batch_size "+batchSize;
//        }
//    }
//    else if(modelType==1){
//        //TODO 此处判断模型类型和数据是否匹配
//        return;
//    }
//    qDebug() << cmd;
//    processTrain->startTrain(modelType, cmd);
}

void ModelTrainPage::stopTrain(){
    processTrain->stopTrain();
}
