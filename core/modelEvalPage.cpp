#include "modelEvalPage.h"

#include "lib/guiLogic/tools/convertTools.h"

#include <QMessageBox>
#include <QFileDialog>

using namespace std;
#define PI 3.1415926

ModelEvalPage::ModelEvalPage(Ui_MainWindow *main_ui,
                             BashTerminal *bash_terminal,
                             DatasetInfo *globalDatasetInfo,
                             ModelInfo *globalModelInfo,
                             TorchServe *globalTorchServe):
    ui(main_ui),
    terminal(bash_terminal),
    datasetInfo(globalDatasetInfo),
    modelInfo(globalModelInfo),
    torchServe(globalTorchServe)
{   
    // 刷新模型、数据集、参数信息
    refreshGlobalInfo();

    // 按钮信号链接
    connect(ui->pushButton_mE_random, &QPushButton::clicked, this, &ModelEvalPage::randSample);
    connect(ui->pushButto_mE_importImg, &QPushButton::clicked, this, &ModelEvalPage::importImage);
    connect(ui->pushButto_mE_importLabel, &QPushButton::clicked, this, &ModelEvalPage::importLabel);
    connect(ui->pushButton_mE_testOneSample, &QPushButton::clicked, this, &ModelEvalPage::testOneSample);

    // 复选框信号链接
    connect(ui->checkBox_mE_showGT, &QCheckBox::stateChanged, this, &ModelEvalPage::checkboxResponse);
    connect(ui->checkBox_mE_showPred, &QCheckBox::stateChanged, this, &ModelEvalPage::checkboxResponse);
}


ModelEvalPage::~ModelEvalPage(){

}


void ModelEvalPage::refreshGlobalInfo(){
    // 基本信息更新
    this->choicedModelName = QString::fromStdString(modelInfo->selectedName);
    this->choicedModelType = QString::fromStdString(modelInfo->selectedType);
    
    ui->label_mE_dataset->setText(QString::fromStdString(datasetInfo->selectedName));
    ui->label_mE_model->setText(choicedModelName);
    ui->label_mE_batch->setText(QString::fromStdString(modelInfo->getAttri(modelInfo->selectedType, modelInfo->selectedName, "batch")));
    this->choicedDatasetPATH = datasetInfo->getAttri(datasetInfo->selectedType,datasetInfo->selectedName,"PATH");

}


int ModelEvalPage::randSample(){
    // 获取所有子文件夹，并判断是否是图片、标注文件夹
    vector<string> allSubDirs;
    dirTools->getDirs(allSubDirs, choicedDatasetPATH);
    vector<string> targetKeys = {"images","labelTxt"};
    for (auto &targetKey: targetKeys){
        if(!(std::find(allSubDirs.begin(), allSubDirs.end(), targetKey) != allSubDirs.end())){
            // 目标路径不存在目标文件夹
            QMessageBox::warning(NULL,"错误","该数据集路径下不存在"+QString::fromStdString(targetKey)+"文件夹！");
            return -1;
        }
    }
    // 获取图片文件夹下的所有图片文件名
    vector<string> imageFileNames;
    dirTools->getFiles(imageFileNames, ".png", choicedDatasetPATH+"/images");
    
    // 随机选取一张图片作为预览图片
    srand((unsigned)time(NULL));
    string choicedImageFile = imageFileNames[(rand())%imageFileNames.size()];
    string choicedImagePath = choicedDatasetPATH+"/images/"+choicedImageFile;
    this->choicedSamplePATH = QString::fromStdString(choicedImagePath);

    // 读取图片
    this->imgSrc = cv::imread(choicedImagePath.c_str(), cv::IMREAD_COLOR);

    // 读取GroundTruth，包含四个坐标和类别信息
    string labelPath = choicedDatasetPATH+"/labelTxt/"+choicedImageFile.substr(0,choicedImageFile.size()-4)+".txt";
    dirTools->getGroundTruth(labels_GT, points_GT, labelPath);

    // 在图片上画出GroundTruth的矩形框
    cv::Mat imgShow = imgSrc.clone();
    this->showImg_GT(imgShow);

    // 清空预测结果
    points_Pred.clear();
    labels_Pred.clear();
    scores_Pred.clear();
}


int ModelEvalPage::importImage(){
    // 手动导入.png/.jpg/.bmp/.tiff/.raw图像
    // FIXME 可能有些图像格式不支持，需要检查一下
    QString filePath = QFileDialog::getOpenFileName(NULL, "导入图片", "./", "Image Files (*.png *.jpg *.bmp *.tiff *.raw)");
    if(filePath.isEmpty()){
        return -1;
    }
    this->choicedSamplePATH = filePath;
    this->imgSrc = cv::imread(filePath.toStdString().c_str(), cv::IMREAD_COLOR);

    // 展示
    this->showImg_Ori();

    // 清空GroundTruth
    labels_GT.clear();
    points_GT.clear();    

    // 清空预测结果
    points_Pred.clear();
    labels_Pred.clear();
    scores_Pred.clear();
}


int ModelEvalPage::importLabel(){
    // 手动导入.txt标注文件
    // FIXME 可能有些图像格式不支持，需要检查一下
    QString filePath = QFileDialog::getOpenFileName(NULL, "导入标注", "./", "Text Files (*.txt)");
    if(filePath.isEmpty()){
        return -1;
    }
    string labelPath = filePath.toStdString();
    dirTools->getGroundTruth(labels_GT, points_GT, labelPath);
    
    // 在图片上画出GroundTruth的矩形框
    cv::Mat imgShow = imgSrc.clone();
    this->showImg_GT(imgShow);
}


int ModelEvalPage::testOneSample(){
    if(!choicedModelName.isEmpty() && !choicedSamplePATH.isEmpty()){
        // 使用TorchServe进行预测
        std::vector<std::map<QString,QString>> predMapStr = torchServe->inferenceOne(
            choicedModelName.split(".mar")[0], 
            choicedModelType, 
            choicedSamplePATH
        );

        // 解析预测结果predMapStr
        if(choicedModelType == "RBOX_DET"){
            // 旋转框检测模型
            for(size_t i = 0; i<predMapStr.size(); i++){
                labels_Pred.push_back(predMapStr[i]["class_name"].toStdString());
                scores_Pred.push_back(predMapStr[i]["score"].toFloat());

                QStringList predCoordStr = predMapStr[i]["bbox"].remove('[').remove(']').split(',');
                cv::Point centerXY = cv::Point(predCoordStr[0].toFloat(), predCoordStr[1].toFloat());
                cv::Size rboxSize = cv::Size(predCoordStr[2].toFloat(), predCoordStr[3].toFloat());
                std::vector<cv::Point> predPointVec;
                calcuRectangle(centerXY, rboxSize, predCoordStr[4].toFloat()*(-1), predPointVec);
                points_Pred.push_back(predPointVec);
            }   
        }
        else if(choicedModelType == "XXX"){
            // 正框检测模型
            // TODO
        }

        // 绘制预测结果到图片上
        cv::Mat imgShow = imgSrc.clone();
        this->showImg_Pred(imgShow);
    }
}


void ModelEvalPage::calcuRectangle(cv::Point centerXY, cv::Size wh, float angle, std::vector<cv::Point> &fourPoints){
    // 计算旋转框四个顶点坐标\

    // 获取原始矩形左上、右上、右下、左下的坐标
    cv::Point point_L_U = cv::Point(centerXY.x - wh.width / 2, centerXY.y - wh.height / 2);	//左上
    cv::Point point_R_U = cv::Point(centerXY.x + wh.width / 2, centerXY.y - wh.height / 2);	//右上
    cv::Point point_R_L = cv::Point(centerXY.x + wh.width / 2, centerXY.y + wh.height / 2);	//右下
    cv::Point point_L_L = cv::Point(centerXY.x - wh.width / 2, centerXY.y + wh.height / 2);	//左下

    std::vector<cv::Point> point = {point_L_U, point_R_U, point_R_L, point_L_L};	//原始矩形数组
    
    //求旋转后的对应坐标
    for (int i = 0; i < 4; i++){		
        int x = point[i].x - centerXY.x;
        int y = point[i].y - centerXY.y;
        fourPoints.push_back(cv::Point(cvRound( x * cos(angle) + y * sin(angle) + centerXY.x),
                                       cvRound(-x * sin(angle) + y * cos(angle) + centerXY.y)));
    }
}


void ModelEvalPage::showImg_Ori(){
    // 将图片显示到界面上
    QPixmap pixmap = CVS::cvMatToQPixmap(this->imgSrc);
    ui->label_mE_img->setPixmap(pixmap.scaled(QSize(700,700), Qt::KeepAspectRatio));
    ui->label_mE_imgName->setText(choicedSamplePATH.split("/").last());
}


void ModelEvalPage::showImg_GT(cv::Mat &img){
    // 绘制旋转框到图片上
    cv::drawContours(img, points_GT, -1, cv::Scalar(16, 124, 16), 2);
    // 绘制类别标签到图片上
    for(size_t i = 0; i<labels_GT.size(); i++){
        cv::putText(img, labels_GT[i], points_GT[i][1], cv::FONT_HERSHEY_COMPLEX, 0.4, cv::Scalar(0, 204, 0), 1);
    }
    // 将图片显示到界面上
    QPixmap pixmap = CVS::cvMatToQPixmap(img);
    ui->label_mE_img->setPixmap(pixmap.scaled(QSize(700,700), Qt::KeepAspectRatio));
    ui->label_mE_imgName->setText(choicedSamplePATH.split("/").last());
}


void ModelEvalPage::showImg_Pred(cv::Mat &img){
    // 绘制旋转框到图片上
    cv::drawContours(img, points_Pred, -1, cv::Scalar(0, 0, 255), 2);
    // 绘制类别标签
    for(size_t i = 0; i<labels_Pred.size(); i++){
        cv::putText(img,
            labels_Pred[i]+": "+std::to_string(scores_Pred[i]).substr(0,5),
            points_Pred[i][1],
            cv::FONT_HERSHEY_COMPLEX,
            0.4, cv::Scalar(0, 0, 255),
            1
        );
    }
    // 将图片显示到界面上
    QPixmap pixmap = CVS::cvMatToQPixmap(img);
    ui->label_mE_img->setPixmap(pixmap.scaled(QSize(700,700), Qt::KeepAspectRatio));
    ui->label_mE_imgName->setText(choicedSamplePATH.split("/").last());

}


void ModelEvalPage::checkboxResponse(){
    // 检查框选择框的状态
    if(!imgSrc.empty()){
        cv::Mat imgShow = imgSrc.clone();
        this->showImg_Ori();
        if(ui->checkBox_mE_showGT->isChecked()){
            if(!points_GT.empty()){
                this->showImg_GT(imgShow);
            }
        }
        if(ui->checkBox_mE_showPred->isChecked()){
            if(!points_Pred.empty()){
                this->showImg_Pred(imgShow);
            }
        }
    }
}