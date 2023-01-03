#include "modelChoicePage.h"

using namespace std;

ModelChoicePage::ModelChoicePage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, ModelInfo *globalModelInfo,TorchServe *globalTorchServe):
    ui(main_ui),
    terminal(bash_terminal),
    modelInfo(globalModelInfo),
    torchServe(globalTorchServe)
{
    // 模型类别选择框事件相应
    BtnGroup_typeChoice->addButton(ui->radioButton__TRA_DL__choice, 0);
    BtnGroup_typeChoice->addButton(ui->radioButton__FEA_RELE__choice, 1);
    BtnGroup_typeChoice->addButton(ui->radioButton__FEW_SHOT__choice, 2);
    BtnGroup_typeChoice->addButton(ui->radioButton__FEA_OPTI__choice, 3);
    BtnGroup_typeChoice->addButton(ui->radioButton__RBOX_DET__choice, 4);
    connect(this->BtnGroup_typeChoice, &QButtonGroup::buttonClicked, this, &ModelChoicePage::changeType);

    // 确定
    connect(ui->pushButton_modelConfirm, &QPushButton::clicked, this, &ModelChoicePage::confirmModel);

    // 保存
    connect(ui->pushButton_saveModelAttri, &QPushButton::clicked, this, &ModelChoicePage::saveModelAttri);

    // 模型属性显示框
    attriLabelGroup["name"] = ui->lineEdit_modelChoice_name;
    attriLabelGroup["algorithm"] = ui->lineEdit_modelChoice_algorithm;
    attriLabelGroup["framework"] = ui->lineEdit_modelChoice_framework;
    attriLabelGroup["class"] = ui->lineEdit_modelChoice_class;
    attriLabelGroup["mAP"] = ui->lineEdit_modelChoice_mAP;
    attriLabelGroup["mAP_50"] = ui->lineEdit_modelChoice_mAP_50;
    attriLabelGroup["trainDataset"] = ui->lineEdit_modelChoice_trainDataset;
    attriLabelGroup["trainEpoch"] = ui->lineEdit_modelChoice_trainEpoch;
    attriLabelGroup["trainLR"] = ui->lineEdit_modelChoice_trainLR;
    attriLabelGroup["PATH"] = ui->lineEdit_modelChoice_PATH;
    attriLabelGroup["batch"] = ui->lineEdit_modelChoice_batch;
    attriLabelGroup["other"] = ui->lineEdit_modelChoice_other;

    processConfirm = new QProcess();
    connect(processConfirm, &QProcess::readyReadStandardOutput, this, &ModelChoicePage::processConfirmFinished);
}

ModelChoicePage::~ModelChoicePage(){
}


void ModelChoicePage::changeType(){
//    this->BtnGroup_typeChoice->checkedId()<<endl;
    // 获取选择的类型内容
    QString selectedType = this->BtnGroup_typeChoice->checkedButton()->objectName().split("__")[1];
    terminal->print("Selected Type: " + selectedType);

    // 更新下拉选择框
    vector<string> comboBoxContents = modelInfo->getNamesInType(
        selectedType.toStdString()
    );
    ui->comboBox_modelNameChoice->clear();
    for(auto &item: comboBoxContents){
        ui->comboBox_modelNameChoice->addItem(QString::fromStdString(item));
    }

}

void ModelChoicePage::execuCmdProcess(QString cmd){
    if(processConfirm->state()==QProcess::Running){
        processConfirm->close();
        processConfirm->kill();
    }
    processConfirm->setProcessChannelMode(QProcess::MergedChannels);
    processConfirm->start(this->terminal->bashApi);
    processConfirm->write(cmd.toLocal8Bit() + '\n');
}

void ModelChoicePage::processConfirmFinished(){
    QByteArray cmdOut = processConfirm->readAllStandardOutput();
    if(!cmdOut.isEmpty()){
        QString logs=QString::fromLocal8Bit(cmdOut);
        if(logs.contains("already") || logs.contains("registered with")){
            terminal->print("模型上传成功！");
            terminal->print(logs);
            torchServe->postTag = 1;
            QMessageBox::warning(NULL,"恭喜","模型上传成功");
            if(processConfirm->state()==QProcess::Running){
                processConfirm->close();
                processConfirm->kill();
            }
        }else if(logs.contains("not found")){
            terminal->print("模型上传失败！");
            QMessageBox::warning(NULL,"错误","模型上传失败");
        }else{
            terminal->print("模型上传中……");
        }
    }
}

void ModelChoicePage::confirmModel(bool notDialog = false){
    // 获取选择的类型内容
    QString selectedType = this->BtnGroup_typeChoice->checkedButton()->objectName().split("__")[1];
    modelInfo->selectedType = selectedType.toStdString(); // save type
    // 获取下拉框内容,即选择模型的名称
    QString selectedName = ui->comboBox_modelNameChoice->currentText();
    modelInfo->selectedName = selectedName.toStdString(); // save name
    terminal->print("Selected Type: " + selectedType + ", Selected Name: " + selectedName);

    if(!selectedType.isEmpty() && !selectedName.isEmpty() && modelInfo->checkMap(modelInfo->selectedType,modelInfo->selectedName,"PATH")){
        // 更新属性显示标签
        updateAttriLabel();
        // 网络图像展示
        QString rootPath = QString::fromStdString(modelInfo->getAttri(modelInfo->selectedType,modelInfo->selectedName,"PATH"));

        // new code 1 added by zyx
        // if(!rootPath.contains("models") || (!rootPath.contains("BBOX")&&!rootPath.contains("RBOX"))){
        if(!rootPath.contains("db/models/BBOX") && !rootPath.contains("db/models/RBOX")){
            QMessageBox::warning(NULL, "模型选择错误", "为确保平台正常运行,请选择规定路径下的模型文件");
            return;
        }
        // new code 1 added by zyx

        QString imgPath = rootPath.split(".mar").first()+".png";
        terminal->print(imgPath);
        ui->label_modelImg->setPixmap(QPixmap(imgPath).scaled(QSize(600,600), Qt::KeepAspectRatio));

        // new code 2 added by zyx
        QStringList rootPathSplit = rootPath.split("/");
        int splitSize = rootPathSplit.size();
        QString parentPath = rootPathSplit[splitSize-2];
        if(parentPath!="BBOX" && parentPath!="RBOX"){
            if(rootPath.contains("BBOX")){
                QString link="../db/models/BBOX/"+selectedName;
                if(access(link.toStdString().c_str(), F_OK) != -1){
                    remove(link.toStdString().c_str());
                }
                QString mkLinkCmd = "ln "+rootPath+" "+link;
                this->terminal->execute(mkLinkCmd);
            }
            else if(rootPath.contains("RBOX")){
                QString link="../db/models/RBOX/"+selectedName;
                if(access(link.toStdString().c_str(), F_OK) != -1){
                    remove(link.toStdString().c_str());
                }
                QString mkLinkCmd = "ln "+rootPath+" "+link;
                this->terminal->execute(mkLinkCmd);
            }
        }
        // new code 2 added by zyx


        if(!notDialog)
            QMessageBox::information(NULL, "模型切换提醒", "已成功切换模型为->"+selectedType+"->"+selectedName+"！");
        QString command = "/root/anaconda3/bin/curl -X POST \"http://localhost:" +
                QString::number(this->torchServe->serverPortList[selectedType]["Management"]) +
                "/models"+"?initial_workers=" +
                QString::number(1) + "&url=" + selectedName + '\"';
        torchServe->postTag = 0;
        if (lastSelectName.isEmpty() || lastSelectType.isEmpty())
        {
            lastSelectName = selectedName;
            lastSelectType = selectedType;

            terminal->print(command);
            this->execuCmdProcess(command);
            // torchServe->postModel(selectedName, selectedType, 1);
        }else{
            if (lastSelectName == selectedName)
            {
                // torchServe->postModel(selectedName, selectedType, 1);
                terminal->print(command);
                this->execuCmdProcess(command);
            }else{
                if(lastSelectType=="RBOX_DET"){
                    QString link="../db/models/RBOX/"+lastSelectName;
                    if(access(link.toStdString().c_str(), F_OK) != -1){
                        remove(link.toStdString().c_str());
                    }
                }
                else{
                    QString link="../db/models/BBOX/"+lastSelectName;
                    if(access(link.toStdString().c_str(), F_OK) != -1){
                        remove(link.toStdString().c_str());
                    }
                }
                torchServe->deleteModel(lastSelectName,lastSelectType);
                // torchServe->postModel(selectedName, selectedType, 1);
                terminal->print(command);
                this->execuCmdProcess(command);
                lastSelectName = selectedName;
                lastSelectType = selectedType;
            }
            

        }
    }
    else{
        if(!notDialog)
            QMessageBox::warning(NULL, "模型切换提醒", "模型切换失败，请指定模型");
    }
}

void ModelChoicePage::updateAttriLabel(){
    if(!modelInfo->checkMap(modelInfo->selectedType, modelInfo->selectedName)){
        return;
    }
    map<string,string> attriContents = modelInfo->getAllAttri(
        modelInfo->selectedType,
        modelInfo->selectedName
    );
    for(auto &currAttriWidget: this->attriLabelGroup){
        currAttriWidget.second->setText(QString::fromStdString(attriContents[currAttriWidget.first]));
    }
    ui->plainTextEdit_modelChoice_note->setPlainText(QString::fromStdString(attriContents["note"]));
}


void ModelChoicePage::saveModelAttri(){
    // 保存至内存
    string type = modelInfo->selectedType;
    string name = modelInfo->selectedName;
    if(!type.empty() && !name.empty()){
        string customAttriValue = "";
        // 对lineEdit组件
        for(auto &currAttriWidget: attriLabelGroup){
            customAttriValue = currAttriWidget.second->text().toStdString();
            if(customAttriValue.empty()){
                customAttriValue = "未定义";
            }
            this->modelInfo->modifyAttri(type, name, currAttriWidget.first, customAttriValue);
        }
        // 对plainTextEdit组件
        customAttriValue = ui->plainTextEdit_modelChoice_note->toPlainText().toStdString();
        if(customAttriValue.empty()){
            customAttriValue = "未定义";
        }
        this->modelInfo->modifyAttri(type, name, "note", customAttriValue);


        // 保存至.xml,并更新
        this->modelInfo->writeToXML(modelInfo->defaultXmlPath);
        this->confirmModel(true);

        // 提醒
        QMessageBox::information(NULL, "属性保存提醒", "模型属性修改已保存");
        terminal->print("模型："+QString::fromStdString(type)+"->"+QString::fromStdString(name)+"->属性修改已保存");
    }
    else{
        QMessageBox::warning(NULL, "属性保存提醒", "属性保存失败，模型未指定！");
        terminal->print("模型："+QString::fromStdString(type)+"->"+QString::fromStdString(name)+"->属性修改无效");
    }

}
