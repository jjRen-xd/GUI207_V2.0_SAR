#include "sensePage.h"
#include <QMessageBox>
#include "lib/guiLogic/tools/convertTools.h""

#include <iostream>
#include <string>
#include <map>


using namespace std;

SenseSetPage::SenseSetPage(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo):
    ui(main_ui),
    terminal(bash_terminal),
    datasetInfo(globalDatasetInfo)
{
    // 数据集类别选择框事件相应
    BtnGroup_typeChoice->addButton(ui->radioButton_BBOX_choice, 0);
    BtnGroup_typeChoice->addButton(ui->radioButton_RBOX_choice, 1);
    connect(this->BtnGroup_typeChoice, &QButtonGroup::buttonClicked, this, &SenseSetPage::changeType);

    // 确定
    connect(ui->pushButton_datasetConfirm, &QPushButton::clicked, this, &SenseSetPage::confirmDataset);

    // 保存
    connect(ui->pushButton_saveDatasetAttri, &QPushButton::clicked, this, &SenseSetPage::saveDatasetAttri);

    // 下一批数据
    connect(ui->pushButton_nextSenseChart, &QPushButton::clicked, this, &SenseSetPage::nextBatchImage);

    // 数据集属性显示框
    this->attriLabelGroup["datasetName"] = ui->lineEdit_sense_datasetName;
    this->attriLabelGroup["datasetType"] = ui->lineEdit_sense_datasetType;
    this->attriLabelGroup["claNum"] = ui->lineEdit_sense_claNum;
    this->attriLabelGroup["targetNumEachCla"] = ui->lineEdit_sense_targetNumEachCla;
    this->attriLabelGroup["imgSize"] = ui->lineEdit_sense_imgSize;
    this->attriLabelGroup["PATH"] = ui->lineEdit_sense_PATH;

    // 图片显示label成组
    imgGroup.push_back(ui->label_datasetClaImg1);
    imgGroup.push_back(ui->label_datasetClaImg2);
    imgGroup.push_back(ui->label_datasetClaImg3);

    imgInfoGroup.push_back(ui->label_datasetCla1);
    imgInfoGroup.push_back(ui->label_datasetCla2);
    imgInfoGroup.push_back(ui->label_datasetCla3);

    // 显示成组
    chartGroup.push_back(ui->label_senseChart1);
    chartGroup.push_back(ui->label_senseChart2);
    chartGroup.push_back(ui->label_senseChart3);
    chartGroup.push_back(ui->label_senseChart4);

    chartInfoGroup.push_back(ui->label_senseChartInfo_1);
    chartInfoGroup.push_back(ui->label_senseChartInfo_2);
    chartInfoGroup.push_back(ui->label_senseChartInfo_3);
    chartInfoGroup.push_back(ui->label_senseChartInfo_4);

}

SenseSetPage::~SenseSetPage(){

}


void SenseSetPage::changeType(){
//    this->BtnGroup_typeChoice->checkedId()<<endl;
    // 获取选择的类型内容
    QString selectedType = this->BtnGroup_typeChoice->checkedButton()->objectName().split("_")[1];
    // terminal->print("Selected Type: " + selectedType);

    // 更新下拉选择框
    vector<string> comboBoxContents = datasetInfo->getNamesInType(
        selectedType.toStdString()
    );
    ui->comboBox_datasetNameChoice->clear();
    for(auto &item: comboBoxContents){
        ui->comboBox_datasetNameChoice->addItem(QString::fromStdString(item));
    }

}


void SenseSetPage::confirmDataset(bool notDialog = false){
    // 获取选择的类型内容
    QString selectedType = this->BtnGroup_typeChoice->checkedButton()->objectName().split("_")[1];
    datasetInfo->selectedType = selectedType.toStdString(); // save type
    // 获取下拉框内容,即选择数据集的名称
    QString selectedName = ui->comboBox_datasetNameChoice->currentText();
    datasetInfo->selectedName = selectedName.toStdString(); // save name
    terminal->print("Selected Type: " + selectedType + ", Selected Name: " + selectedName);
    // cout << "selectedType:" << datasetInfo->selectedType << endl;

    if(!selectedType.isEmpty() && !selectedName.isEmpty()){
        // 更新属性显示标签
        updateAttriLabel();

        // 绘制类别图
        drawClassImage();

        ui->progressBar->setValue(100);

        // 绘制图像
        nextBatchImage();

        if(!notDialog)
            QMessageBox::information(NULL, "数据集切换提醒", "已成功切换数据集为->"+selectedType+"->"+selectedName+"！");
    }
    else{
        if(!notDialog)
            QMessageBox::warning(NULL, "数据集切换提醒", "数据集切换失败，请指定数据集");
    }
}


void SenseSetPage::updateAttriLabel(){
    if(!datasetInfo->checkMap(datasetInfo->selectedType,datasetInfo->selectedName)){
        return;
    }
    map<string,string> attriContents = datasetInfo->getAllAttri(
        datasetInfo->selectedType,
        datasetInfo->selectedName
    );
    for(auto &currAttriWidget: this->attriLabelGroup){
        currAttriWidget.second->setText(QString::fromStdString(attriContents[currAttriWidget.first]));
    }
    ui->plainTextEdit_sense_note->setPlainText(QString::fromStdString(attriContents["note"]));
}


void SenseSetPage::drawClassImage(){
    if(!datasetInfo->checkMap(datasetInfo->selectedType,datasetInfo->selectedName,"PATH")){
        return;
    }
    string rootPath = datasetInfo->getAttri(datasetInfo->selectedType,datasetInfo->selectedName,"PATH");
    string subClassDirName = "classImages";
    // 清空图片显示label
    for(size_t i = 0; i<imgGroup.size(); i++){
        imgGroup[i]->clear();
        imgInfoGroup[i]->clear();
    }
    // 遍历类别图像目录
    vector<string> classNames;
    if(dirTools->getFiles(classNames, ".jpg", rootPath+"/classImages/")){
        for(size_t i=0; i<classNames.size(); i++){
            imgInfoGroup[i]->setText(QString::fromStdString(classNames[i]).split(".")[0]);
            QString imgPath = QString::fromStdString(rootPath+"/classImages/"+classNames[i]);
            imgGroup[i]->setPixmap(QPixmap(imgPath).scaled(QSize(200,150)));
        }
    }

}


// void SenseSetPage::nextBatchImage(){
//     string rootPath = datasetInfo->getAttri(datasetInfo->selectedType,datasetInfo->selectedName,"PATH");
//     // 获取所有子文件夹，并判断是否是图片、标注文件夹
//     vector<string> allSubDirs;
//     dirTools->getDirs(allSubDirs, rootPath);
//     vector<string> targetKeys = {"images","labelTxt"};
//     for (auto &targetKey: targetKeys){
//         if(!(std::find(allSubDirs.begin(), allSubDirs.end(), targetKey) != allSubDirs.end())){
//             // 目标路径不存在目标文件夹
//             QMessageBox::warning(NULL,"错误","该数据集路径下不存在"+QString::fromStdString(targetKey)+"文件夹！");
//             return;
//         }
//     }
//     // 获取图片文件夹下的所有图片文件名
//     vector<string> imageFileNames;
//     dirTools->getFiles(imageFileNames, ".png", rootPath+"/images");

//     for(size_t i = 0; i<chartGroup.size(); i++){
//         // 随机选取一张图片作为预览图片
//         srand((unsigned)time(NULL));
//         string choicedImageFile = imageFileNames[(rand()+i)%imageFileNames.size()];
//         string choicedImagePath = rootPath+"/images/"+choicedImageFile;
//         cv::Mat imgSrc = cv::imread(choicedImagePath.c_str(), cv::IMREAD_COLOR);

//         // 记录GroundTruth，包含四个坐标和类别信息
//         vector<string> label_GT;
//         vector<vector<cv::Point>> points_GT;
//         string labelPath = rootPath+"/labelTxt/"+choicedImageFile.substr(0,choicedImageFile.size()-4)+".txt";
//         dirTools->getGroundTruth(label_GT, points_GT, labelPath);
//         // 绘制旋转框到图片上
//         cv::drawContours(imgSrc, points_GT, -1, cv::Scalar(16, 124, 16), 2);
//         // 绘制类别标签到图片上
//         for(size_t i = 0; i<label_GT.size(); i++){
//             cv::putText(imgSrc, label_GT[i], points_GT[i][1], cv::FONT_HERSHEY_COMPLEX, 0.4, cv::Scalar(0, 204, 0), 1);
//         }
//         // 将图片显示到界面上
//         QPixmap pixmap = CVS::cvMatToQPixmap(imgSrc);
//         chartGroup[i]->setPixmap(pixmap.scaled(QSize(500,500),Qt::KeepAspectRatio));
//         chartInfoGroup[i]->setText("图像名："+QString::fromStdString(choicedImageFile));
//     }
// }


void SenseSetPage::nextBatchImage(){
    if(!datasetInfo->checkMap(datasetInfo->selectedType,datasetInfo->selectedName,"PATH")){
        return;
    }
    string rootPath = datasetInfo->getAttri(datasetInfo->selectedType,datasetInfo->selectedName,"PATH");
    // 获取所有子文件夹，并判断是否是图片、标注文件夹
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
    if(0 == datasetInfo->selectedType.compare("BBOX")){
        dirTools->getFiles(imageFileNames, ".jpg", rootPath+"/images");
    }else{
        dirTools->getFiles(imageFileNames, ".png", rootPath+"/images");
    }


    for(size_t i = 0; i<chartGroup.size(); i++){
        // 随机选取一张图片作为预览图片
        srand((unsigned)time(NULL));
        string choicedImageFile = imageFileNames[(rand()+i)%imageFileNames.size()];
        string choicedImagePath = rootPath+"/images/"+choicedImageFile;
        cv::Mat imgSrc = cv::imread(choicedImagePath.c_str(), cv::IMREAD_COLOR);

        // 记录GroundTruth，包含四个坐标和类别信息
        vector<string> label_GT;
        vector<vector<cv::Point>> points_GT;
        std::vector<std::vector<std::double_t>> bboxGT;
        if(datasetInfo->selectedType == "BBOX"){
            string labelPath = rootPath+"/labelTxt/"+choicedImageFile.substr(0,choicedImageFile.size()-4)+".xml";
            dirTools->getGtXML(label_GT, points_GT,bboxGT, labelPath);
        }else{
            string labelPath = rootPath+"/labelTxt/"+choicedImageFile.substr(0,choicedImageFile.size()-4)+".txt";
            dirTools->getGroundTruth(label_GT, points_GT, labelPath);
        }


        // 绘制框到图片上
        cv::drawContours(imgSrc, points_GT, -1, cv::Scalar(16, 124, 16), 2);
        // 绘制类别标签到图片上
        for(size_t i = 0; i<label_GT.size(); i++){
            cv::putText(imgSrc, label_GT[i], points_GT[i][1], cv::FONT_HERSHEY_COMPLEX, 0.4, cv::Scalar(0, 204, 0), 1);
        }
        // 将图片显示到界面上
        QPixmap pixmap = CVS::cvMatToQPixmap(imgSrc);
        chartGroup[i]->setPixmap(pixmap.scaled(QSize(500,500),Qt::KeepAspectRatio));
        chartInfoGroup[i]->setText("图像名："+QString::fromStdString(choicedImageFile));
    }
}

void SenseSetPage::saveDatasetAttri(){
    // 保存至内存
    string type = datasetInfo->selectedType;
    string name = datasetInfo->selectedName;
    if(!type.empty() && !name.empty()){
        string customAttriValue = "";
        // 对lineEdit组件
        for(auto &currAttriWidget: attriLabelGroup){
            customAttriValue = currAttriWidget.second->text().toStdString();
            if(customAttriValue.empty()){
                customAttriValue = "未定义";
            }
            this->datasetInfo->modifyAttri(type, name, currAttriWidget.first, customAttriValue);
        }
        // 对plainTextEdit组件
        customAttriValue = ui->plainTextEdit_sense_note->toPlainText().toStdString();
        if(customAttriValue.empty()){
            customAttriValue = "未定义";
        }
        this->datasetInfo->modifyAttri(type, name, "note", customAttriValue);


        // 保存至.xml,并更新
        this->datasetInfo->writeToXML(datasetInfo->defaultXmlPath);
        this->confirmDataset(true);

        // 提醒
        QMessageBox::information(NULL, "属性保存提醒", "数据集属性修改已保存");
        terminal->print("数据集："+QString::fromStdString(type)+"->"+QString::fromStdString(name)+"->属性修改已保存");
    }
    else{
        QMessageBox::warning(NULL, "属性保存提醒", "属性保存失败，数据集未指定！");
        terminal->print("数据集："+QString::fromStdString(type)+"->"+QString::fromStdString(name)+"->属性修改无效");
    }

}
