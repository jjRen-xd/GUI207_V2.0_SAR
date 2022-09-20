#include "modelTrainPage.h"

ModelTrainPage::ModelTrainPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal,
 DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo, TorchServe *globalTorchServe, ModelDock *modelDock):
    ui(main_ui),terminal(bash_terminal),
    datasetInfo(globalDatasetInfo),modelInfo(globalModelInfo),
    torchServe(globalTorchServe),modelDock(modelDock){

    processTrain = new QProcess();
    featureBoxs.push_back(ui->f0CheckBox);
    featureBoxs.push_back(ui->f1CheckBox);
    featureBoxs.push_back(ui->f2CheckBox);
    featureBoxs.push_back(ui->f3CheckBox);
    featureBoxs.push_back(ui->f4CheckBox);
    featureBoxs.push_back(ui->f5CheckBox);
    featureBoxs.push_back(ui->f6CheckBox);
    featureBoxs.push_back(ui->f7CheckBox);

    ui->feaScrollArea->setVisible(false);
    ui->fusionTypeBox->setEnabled(false);

    ui->batchsizeEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[1-9][0-9]{1,3}[1-9]$")));
    ui->epochEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^[1-9][0-9]{1,4}[1-9]$")));
    ui->lrEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^0\\.[0-9]{1,5}[1-9]$")));
    ui->saveModelNameEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("[a-zA-Z0-9_]+$")));

    refreshGlobalInfo();

    connect(processTrain, &QProcess::readyReadStandardOutput, this, &ModelTrainPage::readLogOutput);
    connect(ui->startTrainButton, &QPushButton::clicked, this, &ModelTrainPage::startTrain);
    connect(ui->stopTrainButton,  &QPushButton::clicked, this, &ModelTrainPage::stopTrain);
    connect(ui->feaConfusionCheckBox,  &QCheckBox::stateChanged, this, &ModelTrainPage::handFeaAvailable);
}

void ModelTrainPage::refreshGlobalInfo(){
    if(QString::fromStdString(datasetInfo->selectedName)!=""){
        ui->train_data_path->setText(QString::fromStdString(datasetInfo->selectedName));
        this->choicedDatasetPATH = QString::fromStdString(datasetInfo->getAttri(datasetInfo->selectedType,datasetInfo->selectedName,"PATH"));
    }
    else{
        ui->train_data_path->setText("未选择");
        this->choicedDatasetPATH = "";
    }
}

void ModelTrainPage::handFeaAvailable(){
    if(ui->feaConfusionCheckBox->isChecked()){
        ui->feaScrollArea->setVisible(true);
        ui->fusionTypeBox->setEnabled(true);
    }
    else{
        ui->feaScrollArea->setVisible(false);
        ui->fusionTypeBox->setEnabled(false);
    }
}

void ModelTrainPage::startTrain(){
    if(choicedDatasetPATH==""){
        QMessageBox::warning(NULL,"错误","未选择训练数据集!");
        return;
    }
    batchSize = ui->batchsizeEdit->text();
    epoch = ui->epochEdit->text();
    lr = ui->lrEdit->text();
    saveModelName = ui->saveModelNameEdit->text();
    if(batchSize=="" || epoch=="" || lr=="" || saveModelName==""){
        QMessageBox::warning(NULL,"错误","请检查各项文本框中训练参数是否正确配置!");
        return;
    }
    uiInitial();
    ui->startTrainButton->setEnabled(false);
    QString cmd="";
    if(ui->feaConfusionCheckBox->isChecked()){
        this->featureIds="";
        fusionType="two_head_attention";
        QString idCodes="abcdefghijklmnopqrstuvwxyz";
        for(int i=0;i<featureBoxs.size();i++){
            if(featureBoxs[i]->isChecked()){
                featureIds += idCodes[i];
            }
        }
        if(featureIds==""){
           QMessageBox::warning(NULL,"错误","请选择需要关联的特征!");
           return;
        }
        if(ui->fusionTypeBox->currentIndex()==1){
            fusionType="replace_attention";
        }
        modelType="FEA_RELE";
        QDateTime dateTime(QDateTime::currentDateTime());
        time = dateTime.toString("yyyy-MM-dd-hh-mm-ss");
        cmd = "source activate && source deactivate && conda activate 207_base &&"
        " python ../api/bash/mmdetection/GUI/train_model.py --base_cfg_type FeatureConfusion --time "+time+ \
        " --data_root "+choicedDatasetPATH+" --use_feature --confusion_type "+fusionType+ \
        " --feature_ids "+featureIds+" --max_epoch "+epoch+" --batch_size "+batchSize+ \
        " --lr "+lr+" --save_model_name "+saveModelName;
    }
    else{
        modelType="TRA_DL";
        QDateTime dateTime(QDateTime::currentDateTime());
        time = dateTime.toString("yyyy-MM-dd-hh-mm-ss");
        cmd = "source activate && source deactivate && conda activate 207_base &&"
        " python ../api/bash/mmdetection/GUI/train_model.py --base_cfg_type Baseline --time "+time+ \
        " --data_root "+choicedDatasetPATH+" --max_epoch "+epoch+" --batch_size "+batchSize+ \
        " --lr "+lr+" --save_model_name "+saveModelName;
    }
//    qDebug() << cmd;
    execuCmd(cmd);
}

void ModelTrainPage::execuCmd(QString cmd){
  // TODO add code here
    if(processTrain->state()==QProcess::Running){
        processTrain->close();
        processTrain->kill();
    }
    processTrain->setProcessChannelMode(QProcess::MergedChannels);
    processTrain->start(bashApi);
    showLog=true;
    ui->trainLogBrowser->setText("===================Train Starting===================");
    ui->trainProgressBar->setMaximum(0);
    ui->trainProgressBar->setValue(0);
    processTrain->write(cmd.toLocal8Bit() + '\n');
}

void ModelTrainPage::stopTrain(){
    QString cmd="\\x04";
    processTrain->write(cmd.toLocal8Bit() + '\n');
    showLog=false;
    ui->startTrainButton->setEnabled(true);
    ui->trainProgressBar->setMaximum(100);
    ui->trainProgressBar->setValue(0);
    ui->trainLogBrowser->append("===================Train Stoping===================");
    if(processTrain->state()==QProcess::Running){
        processTrain->close();
        processTrain->kill();
    }
}

void ModelTrainPage::readLogOutput(){
    /* 读取终端输出并显示 */
    QByteArray cmdOut = processTrain->readAllStandardOutput();
    if(!cmdOut.isEmpty()){
        QString logs=QString::fromLocal8Bit(cmdOut);
        QStringList lines = logs.split("\n");
        int len=lines.length();
        for(int i=0;i<len;i++){
            QStringList Infos = lines[i].simplified().split(" ");
            if(lines[i].contains("Train Ended",Qt::CaseSensitive)){
                ui->trainLogBrowser->append("===================Train Ended===================");
                showLog=false;
                ui->startTrainButton->setEnabled(true);
            //    导入训练好的模型至系统
                modelDock->importModelAfterTrain(modelType, saveModelName);
                showTrianResult();
                if(processTrain->state()==QProcess::Running){
                    processTrain->close();
                    processTrain->kill();
                }
            }
            else if(lines[i].contains("Failed",Qt::CaseSensitive)){
                ui->startTrainButton->setEnabled(true);
                QDateTime dateTime(QDateTime::currentDateTime());
                ui->trainLogBrowser->append(dateTime.toString("yyyy-MM-dd-hh-mm-ss")+" - 网络模型训练出错：");
                for(i++;i<len;i++){
                    ui->trainLogBrowser->append(lines[i]);
                }
                stopTrain();
            }
            else if(showLog){
                ui->trainLogBrowser->append(lines[i]);
            }
        }
    }
    ui->trainLogBrowser->update();
}

void ModelTrainPage::uiInitial(){
    ui->loss_label->clear();
    ui->acc_label->clear();
    ui->ap_val_label->clear();
    ui->ap_matrix_label->clear();
    ui->trainLogBrowser->clear();
}

void ModelTrainPage::showTrianResult(){
    ui->trainProgressBar->setMaximum(100);
    ui->trainProgressBar->setValue(100);
    //TODO
    QDir dir("../db/traindirs");
    QStringList dirList = dir.entryList(QDir::Dirs);
    foreach (auto dir , dirList){
        if(dir.contains(time)){
            QString wordir    = "../db/traindirs/"+dir;
            QString ap_file   = wordir+"/AP.jpg";
            QString acc_file  = wordir+"/Accuracy.jpg";
            QString loss_file = wordir+"/Loss.jpg";
            QString matrix_file = wordir+"/Matrix.jpg";
            QImage *img_loss = new QImage(loss_file);
            ui->loss_label->setPixmap(QPixmap::fromImage(*img_loss));
            QImage *img_acc = new QImage(acc_file);
            ui->acc_label->setPixmap(QPixmap::fromImage(*img_acc));
            QImage *img_ap = new QImage(ap_file);
            ui->ap_val_label->setPixmap(QPixmap::fromImage(*img_ap));
            QImage *img_cm = new QImage(matrix_file);
            ui->ap_matrix_label->setPixmap(QPixmap::fromImage(*img_cm));
            return;
        }
    }
}

void ModelTrainPage::importModelToSys(QString type, QString modelName){

//    QString modelPath = "../db/models/";

//    // 讲模型导入TorchServe模型库
//    torchServe->postModel(modelName, type, 2);
//    // QString torchServePOST = "curl -X POST \"http://localhost:8081/models?initial_workers=2&url="+modelName+'\"';
//    // terminal->execute(torchServePOST);

//    std::string savePath = modelPath.toStdString();
//    QString rootPath = modelPath.remove(modelPath.length()-modelName.length()-1, modelPath.length());
//    QString xmlPath;

//    std::vector<std::string> allXmlNames;
//    bool existXml = false;
//    dirTools->getFiles(allXmlNames, ".xml",rootPath.toStdString());
//    // 寻找与.mar文件相同命名的.xml文件
//    for(auto &xmlName: allXmlNames){
//        if(QString::fromStdString(xmlName).split(".").first() == modelName.split(".").first()){
//            existXml = true;
//            xmlPath = rootPath + "/" + QString::fromStdString(xmlName);
//            break;
//        }
//    }
//    if(existXml){
//        modelInfo->addItemFromXML(xmlPath.toStdString());

//        terminal->print("添加模型成功:"+xmlPath);
//        QMessageBox::information(NULL, "添加模型", "添加模型成功！");
//    }
//    else{
//        terminal->print("添加模型成功，但该模型没有说明文件.xml！");
//        QMessageBox::warning(NULL, "添加模型", "添加模型成功，但该模型没有说明文件.xml！");
//    }

//    this->modelInfo->modifyAttri(type.toStdString(), modelName.toStdString(), "PATH", savePath);
//    this->reloadTreeView();
//    this->modelInfo->writeToXML(modelInfo->defaultXmlPath);
}
