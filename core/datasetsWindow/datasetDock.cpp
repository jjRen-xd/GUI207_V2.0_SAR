#include "datasetDock.h"

#include "lib/guiLogic/tools/convertTools.h"

#include <QStandardItemModel>
#include <QFileDialog>
#include <QMessageBox>
#include <time.h>

#include <opencv2/opencv.hpp>

using namespace std;

DatasetDock::DatasetDock(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo):
    ui(main_ui),
    terminal(bash_terminal),
    datasetInfo(globalDatasetInfo)
{
    // 数据集导入事件
    connect(ui->action_importDataset_BBOX, &QAction::triggered, this, [this]{importDataset("BBOX");});
    connect(ui->action_importDataset_RBOX, &QAction::triggered, this, [this]{importDataset("RBOX");});

    // 数据集删除事件
    connect(ui->action_Delete_dataset, &QAction::triggered, this, &DatasetDock::deleteDataset);

    // 当前数据集预览树按类型成组
    this->datasetTreeViewGroup["BBOX"] = ui->treeView_BBOX;
    this->datasetTreeViewGroup["RBOX"] = ui->treeView_RBOX;

    // 数据集信息预览label按属性成组
    this->attriLabelGroup["datasetName"] = ui->label_datasetDock_datasetName;
    this->attriLabelGroup["PATH"] = ui->label_datasetDock_PATH;
    this->attriLabelGroup["imgSize"] = ui->label_datasetDock_imgSize;
    this->attriLabelGroup["claNum"] = ui->label_datasetDock_claNum;
    this->attriLabelGroup["targetNumEachCla"] = ui->label_datasetDock_targetNumEachCla;
    this->attriLabelGroup["note"] = ui->label_datasetDock_note;

    // 显示图表成组
    imageViewGroup.push_back(ui->label_datasetDock_imageView1);
    imageViewGroup.push_back(ui->label_datasetDock_imageView2);
    imageInfoGroup.push_back(ui->label_datasetDock_imageInfo1);
    imageInfoGroup.push_back(ui->label_datasetDock_imageInfo2);

    // 初始化TreeView
    reloadTreeView();

    for(auto &currTreeView: datasetTreeViewGroup){
        //链接节点点击事件
        //重复绑定信号和槽函数导致bug弹窗已修复
        connect(currTreeView.second, SIGNAL(clicked(QModelIndex)), this, SLOT(treeItemClicked(QModelIndex)));
    }
}

DatasetDock::~DatasetDock(){

}


void DatasetDock::importDataset(string type){
    QString rootPath = QFileDialog::getExistingDirectory(NULL,"请选择数据集目录","../db/datasets/",QFileDialog::ShowDirsOnly);
    if(rootPath == ""){
        QMessageBox::warning(NULL,"提示","数据集打开失败!");
        return;
    }
    QDir dirs(rootPath);
    QStringList dirList = dirs.entryList(QDir::Dirs);
    if(dirList.indexOf("classImages")==-1 || dirList.indexOf("images")==-1 || dirList.indexOf("labelTxt")==-1){
        QMessageBox::warning(NULL,"提示","请选择正确的数据集文件夹(包含classImages、images、labelTxt等)!");
        return;
    }
    QString datasetName = rootPath.split('/').last();

    vector<string> allXmlNames;
    dirTools->getFiles(allXmlNames, ".xml", rootPath.toStdString());
    if (allXmlNames.empty()){
        terminal->print("添加数据集成功，但该数据集没有说明文件.xml！");
        QMessageBox::warning(NULL, "添加数据集", "添加数据集成功，但该数据集没有说明文件.xml！");
    }
    else{
        QString xmlPath = rootPath + "/" + QString::fromStdString(allXmlNames[0]);
        datasetName = QString::fromStdString(datasetInfo->addItemFromXML(xmlPath.toStdString()));

        terminal->print("添加数据集成功:"+xmlPath);
        QMessageBox::information(NULL, "添加数据集", "添加数据集成功！");
    }
    this->datasetInfo->modifyAttri(type, datasetName.toStdString(),"PATH", rootPath.toStdString());
    this->datasetInfo->modifyAttri(type, datasetName.toStdString(),"claName", rootPath.toStdString());
    this->reloadTreeView();
    this->datasetInfo->print();
    this->datasetInfo->writeToXML(datasetInfo->defaultXmlPath);
}


void DatasetDock::deleteDataset(){
    QMessageBox confirmMsg;
    confirmMsg.setText(QString::fromStdString("确认要删除数据集："+previewType+"->"+previewName));
    confirmMsg.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    if(confirmMsg.exec() == QMessageBox::Yes){
        this->datasetInfo->deleteItem(previewType,previewName);
        this->reloadTreeView();
        this->datasetInfo->writeToXML(datasetInfo->defaultXmlPath);
        for (auto it = this->attriLabelGroup.begin(); it != this->attriLabelGroup.end(); it++) {
            it->second->setText("");
        }
        for(size_t i = 0; i<imageViewGroup.size(); i++){
            imageViewGroup[i]->clear();
            imageInfoGroup[i]->setText("");
        }
        terminal->print(QString::fromStdString("数据集删除成功:"+previewName));
        QMessageBox::information(NULL, "删除数据集", "数据集删除成功！");
    }
    else{}

    return;
}


void DatasetDock::reloadTreeView(){
    for(auto &currTreeView: datasetTreeViewGroup){
        // 不可编辑节点
        currTreeView.second->setEditTriggers(QAbstractItemView::NoEditTriggers);
        currTreeView.second->setHeaderHidden(true);
        // 构建节点
        vector<string> datasetNames = datasetInfo->getNamesInType(currTreeView.first);
        QStandardItemModel *treeModel = new QStandardItemModel(datasetNames.size(),1);
        int idx = 0;
        for(auto &datasetName: datasetNames){
            QStandardItem *nameItem = new QStandardItem(datasetName.c_str());
            treeModel->setItem(idx, 0, nameItem);
            idx += 1;
        }
        currTreeView.second->setModel(treeModel);
    }
}


void DatasetDock::treeItemClicked(const QModelIndex &index){
    // 获取点击预览数据集的类型和名称
    string clickedType = ui->tabWidget_datasetType->currentWidget()->objectName().split("_")[1].toStdString();
    string clickedName = datasetTreeViewGroup[clickedType]->model()->itemData(index).values()[0].toString().toStdString();
    this->previewName = clickedName;
    this->previewType = clickedType;
    // qDebug() << "clickedName:" << QString::fromStdString(clickedName);
    // qDebug() << "clickedType:" << QString::fromStdString(clickedType);
    // 显示数据集预览属性信息
    if(!datasetInfo->checkMap(previewType, previewName)){
        return;
    }
    map<string,string> attriContents = datasetInfo->getAllAttri(previewType, previewName);
    for(auto &currAttriLabel: attriLabelGroup){
        currAttriLabel.second->setText(QString::fromStdString(attriContents[currAttriLabel.first]));
    }

    // 获取所有子文件夹，并判断是否是图片、标注文件夹
    if(!datasetInfo->checkMap(previewType, previewName, "PATH")){
        return;
    }
    string rootPath = datasetInfo->getAttri(previewType, previewName, "PATH");
    vector<string> allSubDirs;
    dirTools->getDirs(allSubDirs, rootPath);
    vector<string> targetKeys = {"images","labelTxt"};
    for (auto &targetKey: targetKeys){
        if(!(std::find(allSubDirs.begin(), allSubDirs.end(), targetKey) != allSubDirs.end())){
            // 目标路径不存在目标文件夹
            QMessageBox::warning(NULL,"错误","该数据集路径下不存在"+QString::fromStdString(targetKey)+"文件夹！");
            return;
        }
    }

    // 获取图片文件夹下的所有图片文件名
    vector<string> imageFileNames;
    if(previewType == "BBOX"){
        dirTools->getFiles(imageFileNames, ".jpg", rootPath+"/images");
    }else{
        dirTools->getFiles(imageFileNames, ".png", rootPath+"/images");
    }


    for(size_t i = 0; i<imageViewGroup.size(); i++){
        // 随机选取一张图片作为预览图片
        srand((unsigned)time(NULL));
        string choicedImageFile = imageFileNames[(rand()+i)%imageFileNames.size()];
        string choicedImagePath = rootPath+"/images/"+choicedImageFile;
        cv::Mat imgSrc = cv::imread(choicedImagePath.c_str(), cv::IMREAD_COLOR);

        // 记录GroundTruth，包含四个坐标和类别信息
        vector<string> label_GT;
        vector<vector<cv::Point>> points_GT;
        std::vector<std::vector<std::double_t>> bboxGT;
        if(previewType == "BBOX"){
            string labelPath = rootPath+"/labelTxt/"+choicedImageFile.substr(0,choicedImageFile.size()-4)+".xml";
            dirTools->getGtXML(label_GT, points_GT,bboxGT, labelPath);
        }else{
            string labelPath = rootPath+"/labelTxt/"+choicedImageFile.substr(0,choicedImageFile.size()-4)+".txt";
            dirTools->getGroundTruth(label_GT, points_GT, labelPath);
        }
        // 绘制旋转框到图片上
        cv::drawContours(imgSrc, points_GT, -1, cv::Scalar(16, 124, 16), 2);
        // 绘制类别标签到图片上
        for(size_t i = 0; i<label_GT.size(); i++){
            cv::putText(imgSrc, label_GT[i], points_GT[i][1], cv::FONT_HERSHEY_COMPLEX, 0.4, cv::Scalar(0, 204, 0), 1);
        }
        // 将图片显示到界面上
        QPixmap pixmap = CVS::cvMatToQPixmap(imgSrc);
        imageViewGroup[i]->setPixmap(pixmap.scaled(QSize(200,200)));
        imageInfoGroup[i]->setText(QString::fromStdString(choicedImageFile));
    }
}
