#include "transferTrainPage.h"

TransferTrainPage::TransferTrainPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal,
 DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo, TorchServe *globalTorchServe, ModelDock *modelDock):
    ui(main_ui),terminal(bash_terminal),
    datasetInfo(globalDatasetInfo),modelInfo(globalModelInfo),
    torchServe(globalTorchServe),modelDock(modelDock){

//    refreshOpticalDataset();
//    refreshSARDataset();
    refreshGlobalInfo();

    batchsizeEdits.push_back(ui->batchsize1);
    batchsizeEdits.push_back(ui->batchsize2);
    batchsizeEdits.push_back(ui->batchsize3);
    epochEdits.push_back(ui->epoch1);
    epochEdits.push_back(ui->epoch2);
    epochEdits.push_back(ui->epoch3);
    lrEdits.push_back(ui->lr1);
    lrEdits.push_back(ui->lr2);
    lrEdits.push_back(ui->lr3);
    processTrain.push_back(new QProcess());
    processTrain.push_back(new QProcess());
    trainProgressBars.push_back(ui->progressBar1);
    trainProgressBars.push_back(ui->progressBar2);
    saveModelNameEdits.push_back(ui->saveModelNameEdit1);
    saveModelNameEdits.push_back(ui->saveModelNameEdit2);

    lossLabel.push_back(ui->fewShotLoss1);
    lossLabel.push_back(ui->fewShotLoss2);
    accLabel.push_back(ui->fewShotAcc1);
    accLabel.push_back(ui->fewShotAcc2);
    apValLabel.push_back(ui->fewShotApVal1);
    apValLabel.push_back(ui->fewShotApVal2);
    conMatrixLabel.push_back(ui->fewShotMatrix1);
    conMatrixLabel.push_back(ui->fewShotMatrix2);
    starTrainBts.push_back(ui->startNormalTrainButton);
    starTrainBts.push_back(ui->startTransferTrainButton);
    ui->fewshotVolumeEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[1-9][0-9]$")));

    for(int i=0;i<3;i++){
        if(i<2){
            times.push_back("");
            saveModelNames.push_back("");
            saveModelNameEdits[i]->setValidator(new QRegularExpressionValidator(QRegularExpression("^[a-zA-Z][a-zA-Z0-9_]{0,}[a-zA-Z0-9]$")));
            processTrain[i]->setProcessChannelMode(QProcess::MergedChannels);
        }
        batchsizes.push_back("");
        epochs.push_back("");
        lrs.push_back("");
        batchsizeEdits[i]->setValidator(new QRegularExpressionValidator(QRegularExpression("^[1-9][0-9]{1,3}[1-9]$")));
        epochEdits[i]->setValidator(new QRegularExpressionValidator(QRegularExpression("^[1-9][0-9]{1,4}[1-9]$")));
        lrEdits[i]->setValidator(new QRegularExpressionValidator(QRegularExpression("^0\\.[0-9]{1,5}[1-9]$")));
    }

//    connect(ui->action_importDataset_BBOX, &QAction::triggered, this, &TransferTrainPage::refreshOpticalDataset);
//    connect(ui->action_Delete_dataset, &QAction::triggered, this, &TransferTrainPage::refreshOpticalDataset);
    connect(ui->action_importDataset_BBOX, &QAction::triggered, this, &TransferTrainPage::refreshGlobalInfo);
    connect(ui->action_Delete_dataset, &QAction::triggered, this, &TransferTrainPage::refreshGlobalInfo);
    connect(ui->startNormalTrainButton, &QPushButton::clicked, this, &TransferTrainPage::startNormalTrain);
    connect(ui->startTransferTrainButton, &QPushButton::clicked, this, &TransferTrainPage::startTransferTrain);

    connect(processTrain[0], &QProcess::readyReadStandardOutput, this, [this]{monitorTrainProcess(0);});
    connect(processTrain[1], &QProcess::readyReadStandardOutput, this, [this]{monitorTrainProcess(1);});
}


TransferTrainPage::~TransferTrainPage(){
    for(int i=0;i<2;i++){
        if(processTrain[i]->state()==QProcess::Running){
            processTrain[i]->terminate();
            processTrain[i]->close();
            processTrain[i]->kill();
        }
    }
}

void TransferTrainPage::refreshGlobalInfo(){
    // 更新下拉选择框
    ui->opticalDataBox->clear();
    std::vector<std::string> optComboBoxContents = datasetInfo->getNamesInType(optivalDataType.toStdString());
    for(auto &item: optComboBoxContents){
        ui->opticalDataBox->addItem(QString::fromStdString(item));
    }

    // 更新下拉选择框
    ui->sarDataBox->clear();
    std::vector<std::string> sarComboBoxContents = datasetInfo->getNamesInType(sarDataType.toStdString());
    for(auto &item: sarComboBoxContents){
        ui->sarDataBox->addItem(QString::fromStdString(item));
    }

    // 更新下拉选择框
//    ui->existedModelBox->clear();
//    std::vector<std::string> modelComboBoxContents = modelInfo->getNamesInType("FEW_SHOT");
//    for(auto &item: modelComboBoxContents){
//        ui->existedModelBox->addItem(QString::fromStdString(item));
//    }
}

void TransferTrainPage::startNormalTrain(){
    uiInitial(0);
    if(!datasetInfo->checkMap(sarDataType.toStdString(),ui->sarDataBox->currentText().toStdString(),"PATH")){
        QMessageBox::warning(NULL,"错误","请选择可用目标域数据集!");
        return;
    }
    choicedSARData = QString::fromStdString(datasetInfo->getAttri(sarDataType.toStdString(),
                                               ui->sarDataBox->currentText().toStdString(),"PATH"));
    volume = ui->fewshotVolumeEdit->text();
    batchsizes[0] = batchsizeEdits[0]->text();
    epochs[0] = epochEdits[0]->text();
    lrs[0] = lrEdits[0]->text();
    saveModelNames[0] = saveModelNameEdits[0]->text();
    if(batchsizes[0]=="" || epochs[0]=="" || lrs[0]=="" || saveModelNames[0]=="" || volume==""){
        QMessageBox::warning(NULL,"错误","请检查各项文本框中训练参数是否正确配置!");
        return;
    }
    ui->startNormalTrainButton->setEnabled(false);
    QString cmd="";
    if(processTrain[0]->state()!=QProcess::Running){
        cmd = "source activate && source deactivate && conda activate 207_base && ";
    }
    QDateTime dateTime(QDateTime::currentDateTime());
    times[0] = dateTime.toString("yyyy-MM-dd-hh-mm-ss");
    cmd += "python ../db/bash/mmdetection/GUI/train_model.py --base_cfg_type FewShot --time "+times[0]+ \
    " --data_root "+choicedSARData+" --max_epoch "+epochs[0]+" --batch_size "+batchsizes[0]+ \
    " --lr "+lrs[0]+" --volume "+volume+" --save_model_name "+saveModelNames[0];

    this->terminal->print(cmd);
    execuCmd(0, cmd);
}

void TransferTrainPage::startTransferTrain(){
    uiInitial(1);
    if(!datasetInfo->checkMap(optivalDataType.toStdString(),ui->opticalDataBox->currentText().toStdString(),"PATH")){
        QMessageBox::warning(NULL,"错误","请选择可用源域数据集!");
        return;
    }
    choicedOpticalData = QString::fromStdString(datasetInfo->getAttri(optivalDataType.toStdString(),
                                               ui->opticalDataBox->currentText().toStdString(),"PATH"));

    if(!datasetInfo->checkMap(sarDataType.toStdString(),ui->sarDataBox->currentText().toStdString(),"PATH")){
        QMessageBox::warning(NULL,"错误","请选择可用目标域数据集!");
        return;
    }
    choicedSARData = QString::fromStdString(datasetInfo->getAttri(sarDataType.toStdString(),
                                               ui->sarDataBox->currentText().toStdString(),"PATH"));
    volume = ui->fewshotVolumeEdit->text();
    for(int i=1;i<3;i++){
        batchsizes[i] = batchsizeEdits[i]->text();
        epochs[i] = epochEdits[i]->text();
        lrs[i] = lrEdits[i]->text();
    }
    saveModelNames[1] = saveModelNameEdits[1]->text();
    
    QString cmd="";
    if(processTrain[1]->state()!=QProcess::Running){
        cmd = "source activate && source deactivate && conda activate 207_base && ";
    }
//    if(ui->useExistedModelBox->isChecked()){
//        choicedPreModel = QString::fromStdString(modelInfo->getAttri("FEW_SHOT",ui->existedModelBox->currentText().toStdString(),"PATH"));
//        //"../db/models/doship_12epoch.pth";
//        if(batchsizes[2]=="" || epochs[2]=="" || lrs[2]=="" || saveModelNames[2]==""){
//            QMessageBox::warning(NULL,"错误","请检查各项文本框中训练参数是否正确配置!");
//            return;
//        }
//        ui->startTransferTrainButton->setEnabled(false);
//        QDateTime dateTime(QDateTime::currentDateTime());
//        times[1] = dateTime.toString("yyyy-MM-dd-hh-mm-ss");
//        cmd += "python ../db/bash/mmdetection/GUI/train_model.py --base_cfg_type FewShot --time "+times[1]+ \
//        " --data_root "+choicedSARData+" --max_epoch "+epochs[2]+" --batch_size "+batchsizes[2]+ \
//        " --lr "+lrs[2]+" --volume "+volume+" --load_from "+choicedPreModel+" --save_model_name "+saveModelNames[1];
//    }
//    else{
    if(batchsizes[1]=="" || epochs[1]=="" || lrs[1]=="" ||  batchsizes[2]=="" ||
        epochs[2]=="" || lrs[2]=="" || saveModelNames[1]=="" || volume==""){
        QMessageBox::warning(NULL,"错误","请检查各项文本框中训练参数是否正确配置!");
        return;
    }
    ui->startTransferTrainButton->setEnabled(false);
    QDateTime dateTime(QDateTime::currentDateTime());
    times[1] = dateTime.toString("yyyy-MM-dd-hh-mm-ss");
    cmd += "python ../db/bash/mmdetection/GUI/train_model.py --base_cfg_type FewShot --time "+times[1]+ \
    " --data_root "+choicedOpticalData+" --max_epoch "+epochs[1]+" --batch_size "+batchsizes[1]+ \
    " --lr "+lrs[1]+" --volume "+volume+" --save_model_name "+saveModelNames[1]+" --endmessage next_step && "+ \
    " python ../db/bash/mmdetection/GUI/train_model.py --base_cfg_type FewShot --time "+times[1]+ \
    " --data_root "+choicedSARData+" --max_epoch "+epochs[2]+" --batch_size "+batchsizes[2]+ \
    " --lr "+lrs[2]+" --volume "+volume+" --load_from last_step_"+epochs[1]+" --save_model_name "+saveModelNames[1];
//    }

    this->terminal->print(cmd);
    execuCmd(1, cmd);
}

void TransferTrainPage::execuCmd(int modeltypeId, QString cmd){
    if(processTrain[modeltypeId]->state()!=QProcess::Running){
        processTrain[modeltypeId]->start(bashApi);
    }
    trainProgressBars[modeltypeId]->setMaximum(0);
    trainProgressBars[modeltypeId]->setValue(0);
    processTrain[modeltypeId]->write(cmd.toLocal8Bit() + '\n');
}

void TransferTrainPage::monitorTrainProcess(int modeltypeId){
    /* 读取终端输出并显示 */
    QByteArray cmdOut = processTrain[modeltypeId]->readAllStandardOutput();
    if(!cmdOut.isEmpty()){
        QString logs=QString::fromLocal8Bit(cmdOut);
        QStringList lines = logs.split("\n");
        int len=lines.length();
        for(int i=0;i<len;i++){
            if(lines[i].contains("Train Ended",Qt::CaseSensitive)){
                starTrainBts[modeltypeId]->setEnabled(true);
                trainProgressBars[modeltypeId]->setMaximum(100);
                trainProgressBars[modeltypeId]->setValue(100);
                showTrianResult(modeltypeId);
//                if(processTrain[modeltypeId]->state()==QProcess::Running){
//                    processTrain[modeltypeId]->close();
//                    processTrain[modeltypeId]->kill();
//                }
            }
            else if(lines[i].contains("Failed",Qt::CaseSensitive)){
                QDateTime dateTime(QDateTime::currentDateTime());
                if(!modeltypeId){
                    ui->textEdit->append(dateTime.toString("yyyy-MM-dd-hh-mm-ss")+" - 小样本直接训练出错：");
                }
                else{
                    ui->textEdit->append(dateTime.toString("yyyy-MM-dd-hh-mm-ss")+" - 小样本迁移训练出错：");
                }
                for(i++;i<len;i++){
                    ui->textEdit->append(lines[i]);
                }
                ui->textEdit->append("[注] 学习率较大或样本容量较小都会导致训练失败");
                ui->textEdit->update();
                starTrainBts[modeltypeId]->setEnabled(true);
                trainProgressBars[modeltypeId]->setMaximum(100);
                trainProgressBars[modeltypeId]->setValue(0);
//                if(processTrain[modeltypeId]->state()==QProcess::Running){
//                    processTrain[modeltypeId]->close();
//                    processTrain[modeltypeId]->kill();
//                }
            }
        }
    }
}

void TransferTrainPage::uiInitial(int modeltypeId){
    lossLabel[modeltypeId]->clear();
    accLabel[modeltypeId]->clear();
    apValLabel[modeltypeId]->clear();
    conMatrixLabel[modeltypeId]->clear();
}

void TransferTrainPage::showTrianResult(int modeltypeId){
    QDir dir("../db/models/BBOX");
    QStringList dirList = dir.entryList(QDir::Dirs);
    foreach (auto dir , dirList){
        if(dir.contains(times[modeltypeId])){
            QString wordir    = "../db/models/BBOX/"+dir;
            QString ap_file   = wordir+"/AP.jpg";
            QString acc_file  = wordir+"/Accuracy.jpg";
            QString loss_file = wordir+"/Loss.jpg";
            QString matrix_file = wordir+"/Matrix.jpg";
            QImage *img_loss = new QImage(loss_file);
            lossLabel[modeltypeId]->setPixmap(QPixmap::fromImage(*img_loss));
            QImage *img_acc = new QImage(acc_file);
            accLabel[modeltypeId]->setPixmap(QPixmap::fromImage(*img_acc));
            QImage *img_ap = new QImage(ap_file);
            apValLabel[modeltypeId]->setPixmap(QPixmap::fromImage(*img_ap));
            QImage *img_cm = new QImage(matrix_file);
            conMatrixLabel[modeltypeId]->setPixmap(QPixmap::fromImage(*img_cm));

            //    导入训练好的模型至系统
            modelDock->importModelAfterTrain(modelType, wordir, saveModelNames[modeltypeId], ".mar");
            return;
        }
    }
}
