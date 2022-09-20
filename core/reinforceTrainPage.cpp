#include "reinforceTrainPage.h"

ReinfoceTrainPage::ReinfoceTrainPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal,
 DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo, TorchServe *globalTorchServe, ModelDock *modelDock):
    ui(main_ui),terminal(bash_terminal),
    datasetInfo(globalDatasetInfo),modelInfo(globalModelInfo),
    torchServe(globalTorchServe),modelDock(modelDock){

    processTrain = new QProcess();
    featureWeightEdits.push_back(ui->f0_weight_edit);
    featureWeightEdits.push_back(ui->f1_weight_edit);
    featureWeightEdits.push_back(ui->f2_weight_edit);
    featureWeightEdits.push_back(ui->f3_weight_edit);
    featureWeightEdits.push_back(ui->f4_weight_edit);
    featureWeightEdits.push_back(ui->f5_weight_edit);
    featureWeightEdits.push_back(ui->f6_weight_edit);
    featureWeightEdits.push_back(ui->f7_weight_edit);

    ui->reinforceBatchsizeEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[1-9][0-9]{1,3}[1-9]$")));
    ui->reinforceEpochEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[1-9][0-9]{1,4}[1-9]$")));
    ui->reinforceLrEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^0\\.[0-9]{1,5}[1-9]$")));
    ui->reinforceSaveModelNameEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[a-zA-Z0-9_]+$")));

    refreshDataInfo();

    connect(ui->action_importDataset_BBOX, &QAction::triggered, this, &ReinfoceTrainPage::refreshDataInfo);
    connect(ui->action_Delete_dataset, &QAction::triggered, this, &ReinfoceTrainPage::refreshDataInfo);

    connect(processTrain, &QProcess::readyReadStandardOutput, this, &ReinfoceTrainPage::readLogOutput);
    connect(ui->startRfTrainButton, &QPushButton::clicked, this, &ReinfoceTrainPage::startTrain);
    connect(ui->stopRfTrainButton,  &QPushButton::clicked, this, &ReinfoceTrainPage::stopTrain);
}

void ReinfoceTrainPage::refreshDataInfo(){
    ui->reinforceDataBox->clear();
    std::vector<std::string> comboBoxContents = datasetInfo->getNamesInType(reinforceDataType.toStdString());
    for(auto &item: comboBoxContents){
        ui->reinforceDataBox->addItem(QString::fromStdString(item));
    }
}

void ReinfoceTrainPage::startTrain(){
    choicedDatasetPATH = QString::fromStdString(datasetInfo->getAttri(reinforceDataType.toStdString(),
                                               ui->opticalDataBox->currentText().toStdString(),"PATH"));
    batchSize = ui->reinforceBatchsizeEdit->text();
    epoch = ui->reinforceEpochEdit->text();
    lr = ui->reinforceLrEdit->text();
    saveModelName = ui->reinforceSaveModelNameEdit->text();
    QString cmd="";
    if(!ui->useTrainedRfModelCheckBox->isChecked()){
        if(batchSize=="" || epoch=="" || lr=="" || saveModelName==""){
            QMessageBox::warning(NULL,"错误","请检查各项文本框中训练参数是否正确配置!");
            return;
        }
        QDateTime dateTime(QDateTime::currentDateTime());
        time = dateTime.toString("yyyy-MM-dd-hh-mm-ss");
        cmd = "source activate && source deactivate && conda activate 207_base &&"
        " python ../api/bash/mmdetection/GUI/DQNtrain.py --base_cfg_type ReinforceLearning --time "+time+ \
        " --data_root "+choicedDatasetPATH+" --max_epoch "+epoch+" --batch_size "+batchSize+" --lr "+lr+ \
        " --save_model_name "+saveModelName+" --num_of_state "+QString::number(featureWeightEdits.size());
    }
    else{
        choicedModelPATH = "../db/traindirs/pretrained/dqn.pth";
        cmd = "source activate && source deactivate && conda activate 207_base && python ../api/bash/mmdetection/GUI/DQNtrain.py"
        " --load_from "+choicedModelPATH+" --num_of_state "+QString::number(featureWeightEdits.size());
        execuCmd(cmd);
    }
    qDebug() << cmd;
//    execuCmd(cmd);
}

void ReinfoceTrainPage::stopTrain(){
    QString cmd="\\x04";
    processTrain->write(cmd.toLocal8Bit() + '\n');
    ui->startRfTrainButton->setEnabled(true);
    ui->reinforceTrainBar->setMaximum(100);
    ui->reinforceTrainBar->setValue(0);
    if(processTrain->state()==QProcess::Running){
        processTrain->close();
        processTrain->kill();
    }
}

void ReinfoceTrainPage::execuCmd(QString cmd){
  // TODO add code here
    if(processTrain->state()==QProcess::Running){
        processTrain->close();
        processTrain->kill();
    }
    ui->startRfTrainButton->setEnabled(false);
    ui->reinforceTrainBar->setMaximum(0);
    ui->reinforceTrainBar->setValue(0);
    for(int i=0;i<featureWeightEdits.size();i++){
        featureWeightEdits[i]->setText("");
    }
    processTrain->setProcessChannelMode(QProcess::MergedChannels);
    processTrain->start(bashApi);
    processTrain->write(cmd.toLocal8Bit() + '\n');
}

void ReinfoceTrainPage::readLogOutput(){
/* 读取终端输出并显示 */
    QByteArray cmdOut = processTrain->readAllStandardOutput();
    if(!cmdOut.isEmpty()){
        QString logs=QString::fromLocal8Bit(cmdOut);
        QStringList lines = logs.split("\n");
        int len=lines.length();
        for(int i=0;i<len;i++){
            if(lines[i].contains("Train Ended",Qt::CaseSensitive)){
                ui->startRfTrainButton->setEnabled(true);
                ui->reinforceTrainBar->setMaximum(100);
                ui->reinforceTrainBar->setValue(100);
                modelDock->importModelAfterTrain(modelType, saveModelName, ".pth");
                if(processTrain->state()==QProcess::Running){
                    processTrain->close();
                    processTrain->kill();
                }
            }
            else if(lines[i].contains("Failed",Qt::CaseSensitive)){
                QDateTime dateTime(QDateTime::currentDateTime());
                ui->textEdit->append(dateTime.toString("yyyy-MM-dd-hh-mm-ss")+" - 特征强化学习训练出错：");
                for(i++;i<len;i++){
                    ui->textEdit->append(lines[i]);
                }
                // ui->textEdit->append("[注] 学习率较大或样本容量较小都会导致训练失败");
                ui->textEdit->update();
                ui->startRfTrainButton->setEnabled(true);
                ui->reinforceTrainBar->setMaximum(100);
                ui->reinforceTrainBar->setValue(0);
                if(processTrain->state()==QProcess::Running){
                    processTrain->close();
                    processTrain->kill();
                }
            }
            else if(lines[i].contains("result",Qt::CaseSensitive)){
                showResult(lines[i]);
            }
        }
    }
}

void ReinfoceTrainPage::showResult(QString resultline){
    QStringList list = resultline.split(":");
    int id = list[0].split("_")[1].toInt();
    featureWeightEdits[id]->setText(list[1]);
}
