#include "modelDock.h"

#include <QStandardItemModel>
#include <QFileDialog>
#include <QMessageBox>

using namespace std;

ModelDock::ModelDock(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, ModelInfo *globalModelInfo, TorchServe *globalTorchServe):
    ui(main_ui),
    terminal(bash_terminal),
    modelInfo(globalModelInfo),
    torchServe(globalTorchServe)
{
    // 模型导入事件
    connect(ui->action_importModel_RBOX_DET, &QAction::triggered, this, [this]{importModel("RBOX_DET");});
    connect(ui->action_importModel_TRA_DL, &QAction::triggered, this, [this]{importModel("TRA_DL");});
    connect(ui->action_importModel_FEA_RELE, &QAction::triggered, this, [this]{importModel("FEA_RELE");});
    connect(ui->action_importModel_FEW_SHOT, &QAction::triggered, this, [this]{importModel("FEW_SHOT");});
    connect(ui->action_importModel_FEA_OPTI, &QAction::triggered, this, [this]{importModel("FEA_OPTI");});
    // 模型删除事件
    connect(ui->action_dele_model, &QAction::triggered, this, &ModelDock::deleteModel);

    // 预览属性标签成组
    attriLabelGroup["name"] = ui->label_modelDock_name;
    attriLabelGroup["PATH"] = ui->label_modelDock_PATH;
    attriLabelGroup["algorithm"] = ui->label_modelDock_algorithm;
    attriLabelGroup["framework"] = ui->label_modelDock_framework;
    attriLabelGroup["class"] = ui->label_modelDock_class;
    attriLabelGroup["mAP"] = ui->label_modelDock_mAP;
    attriLabelGroup["mAP_50"] = ui->label_modelDock_mAP_50;
    attriLabelGroup["trainDataset"] = ui->label_modelDock_trainDataset;
    attriLabelGroup["trainEpoch"] = ui->label_modelDock_trainEpoch;
    attriLabelGroup["trainLR"] = ui->label_modelDock_trainLR;
    attriLabelGroup["note"] = ui->label_modelDock_note;

    // TreeView
    this->modelTreeView = ui->TreeView_modelDock;
    // 初始化TreeView
    reloadTreeView();
}


void ModelDock::reloadTreeView(){
    // 不可编辑节点
    modelTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    modelTreeView->setHeaderHidden(true);
    // 构建节点
    vector<string> modelTypes = modelInfo->getTypes();
    QStandardItemModel *typeTreeModel = new QStandardItemModel(modelTypes.size(),1);
    int idx = 0;
    for(auto &type: modelTypes){
        QStandardItem *typeItem = new QStandardItem(modelInfo->var2TypeName[type].c_str());
        typeTreeModel->setItem(idx, 0, typeItem);   // 节点链接

        // 构建子节点
        vector<string> modelNames = modelInfo->getNamesInType(type);
        for(auto &name: modelNames){
            QStandardItem *nameItem = new QStandardItem(QString::fromStdString(name));
            typeItem->appendRow(nameItem);
        }

        idx += 1;
    }
    modelTreeView->setModel(typeTreeModel);
    //链接节点点击事件
    connect(modelTreeView, SIGNAL(clicked(QModelIndex)), this, SLOT(treeItemClicked(QModelIndex)));
    // terminal->print("i am here!");
}


void ModelDock::treeItemClicked(const QModelIndex &index){
    // 获取点击预览模型的类型和名称
    QStandardItem *currItem = static_cast<QStandardItemModel*>(modelTreeView->model())->itemFromIndex(index);     //返回给定index的条目
    auto parentItem = currItem->parent();
    if(!parentItem){    // 点击父节点直接返回
        return;
    }

    string clickedName = currItem->data(0).toString().toStdString();             //获取该条目的值
    string clickedType = modelInfo->typeName2Var[currItem->parent()->data(0).toString().toStdString()];
    this->previewName = clickedName;
    this->previewType = clickedType;
    terminal->print(QString::fromStdString(clickedName));
    terminal->print(QString::fromStdString(clickedType));

    // 更新预览属性参数return infoMap[Type][Name];
    if(!modelInfo->checkMap(previewType, previewName)){
        return;
    }
    map<string,string> attriContents = modelInfo->getAllAttri(previewType, previewName);
    for(auto &currAttriLabel: attriLabelGroup){
        currAttriLabel.second->setText(QString::fromStdString(attriContents[currAttriLabel.first]));
    }
}


void ModelDock::importModel(string type){
    QString modelPath = "";
    if(type=="FEA_OPTI"){
        modelPath = QFileDialog::getOpenFileName(NULL, "打开网络模型文件", "../db/models/", "Mar files(*.pth)");
    }
    else{
        modelPath = QFileDialog::getOpenFileName(NULL, "打开网络模型文件", "../db/models/", "Mar files(*.mar)");
    }
//    qDebug() << modelPath;
    if(modelPath == ""){
        QMessageBox::warning(NULL, "提示", "文件打开失败!");
        return;
    }
    QString modelName = modelPath.split('/').last();

    // 讲模型导入TorchServe模型库
//    if(type!="FEA_OPTI"){
//        torchServe->postModel(modelName, QString::fromStdString(type), 2);
//    }
    // QString torchServePOST = "curl -X POST \"http://localhost:8081/models?initial_workers=2&url="+modelName+'\"';
    // terminal->execute(torchServePOST);
    string savePath = modelPath.toStdString();
    QString rootPath = modelPath.remove(modelPath.length()-modelName.length()-1, modelPath.length());
//    qDebug() << rootPath;
    QString xmlPath;
    vector<string> allXmlNames;
    bool existXml = false;
    dirTools->getFiles(allXmlNames, ".xml",rootPath.toStdString());
    // 寻找与.mar文件相同命名的.xml文件
    for(auto &xmlName: allXmlNames){
        std::cout << xmlName;
        if(QString::fromStdString(xmlName).split(".").first() == modelName.split(".").first()){
            existXml = true;
            xmlPath = rootPath + "/" + QString::fromStdString(xmlName);
            break;
        }
    }
    if(existXml){
        string modelType = modelInfo->addItemFromXML(xmlPath.toStdString());
        if(modelType != type){
            QMessageBox::warning(NULL, "添加模型", "添加模型失败，模型信息与所选类型不符！");
            this->modelInfo->loadFromXML(modelInfo->defaultXmlPath);
            return;
        }
        terminal->print("添加模型成功:"+xmlPath);
        QMessageBox::information(NULL, "添加模型", "添加模型成功！");
    }
    else{
        terminal->print("添加模型成功，但该模型没有说明文件.xml！");
        QMessageBox::warning(NULL, "添加模型", "添加模型成功，但该模型没有说明文件.xml！");
    }
    this->modelInfo->modifyAttri(type, modelName.toStdString(), "PATH", savePath);
    this->reloadTreeView();
    this->modelInfo->writeToXML(modelInfo->defaultXmlPath);
}


void ModelDock::importModelAfterTrain(QString type, QString modelPath, QString modelName, QString modelSuffix){
    // 模型导入TorchServe模型库
    if(modelSuffix==".mar"){
        modelPath = modelPath+"/"+modelName+".mar";
    }
    else{
        modelPath = modelPath+"/"+modelName+".pth";
    }
    modelName = modelPath.split('/').last();

    string savePath = modelPath.toStdString();
    QString rootPath = modelPath.remove(modelPath.length()-modelName.length()-1, modelPath.length());
    QString xmlPath;

    vector<string> allXmlNames;
    bool existXml = false;
    dirTools->getFiles(allXmlNames, ".xml",rootPath.toStdString());
    // 寻找与.mar文件相同命名的.xml文件
    for(auto &xmlName: allXmlNames){
        if(QString::fromStdString(xmlName).split(".").first() == modelName.split(".").first()){
            existXml = true;
            xmlPath = rootPath + "/" + QString::fromStdString(xmlName);
            break;
        }
    }
    if(existXml){
        modelInfo->addItemFromXML(xmlPath.toStdString());

        terminal->print("添加模型成功:"+xmlPath);
        QMessageBox::information(NULL, "添加模型", "添加模型成功！");
    }
    else{
        terminal->print("添加模型成功，但该模型没有说明文件.xml！");
        QMessageBox::warning(NULL, "添加模型", "添加模型成功，但该模型没有说明文件.xml！");
    }
    this->modelInfo->modifyAttri(type.toStdString(), modelName.toStdString(), "PATH", savePath);
    this->reloadTreeView();
    this->modelInfo->writeToXML(modelInfo->defaultXmlPath);
}

void ModelDock::deleteModel(){
    QMessageBox confirmMsg;
    confirmMsg.setText(QString::fromStdString("确认要删除模型："+previewType+"->"+previewName));
    confirmMsg.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    if(confirmMsg.exec() == QMessageBox::Yes){
        // new code added by zyx
        if(this->previewType=="RBOX_DET"){
            std::string link="../db/models/RBOX/"+previewName;
            if(access(link.c_str(), F_OK) != -1){
                remove(link.c_str());
            }
        }
        else{
            std::string link="../db/models/BBOX/"+previewName;
            if(access(link.c_str(), F_OK) != -1){
                remove(link.c_str());
            }
        }
        // new code added by zyx
        // 从TorchServe模型库删除模型
        torchServe->deleteModel(QString::fromStdString(previewName), QString::fromStdString(previewType));
        // QString torchServeDELETE = "curl -X DELETE http://localhost:8081/models/" +
        //                             QString::fromStdString(previewName).split(".")[0];
        // terminal->print(torchServeDELETE);
        //  terminal->execute(torchServeDELETE);
        this->modelInfo->deleteItem(previewType,previewName);
        this->reloadTreeView();
        this->modelInfo->writeToXML(modelInfo->defaultXmlPath);
        for (auto it = this->attriLabelGroup.begin(); it != this->attriLabelGroup.end(); it++) {
            it->second->setText("");
        }
        terminal->print(QString::fromStdString("模型删除成功:"+previewName));
        QMessageBox::information(NULL, "删除模型", "模型删除成功！");
    }
    else{}

    return;
}
