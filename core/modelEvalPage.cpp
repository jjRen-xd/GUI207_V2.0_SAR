#include "modelEvalPage.h"

#include "lib/guiLogic/tools/convertTools.h"

#include <QMessageBox>

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
    connect(ui->pushButto_mE_importImg, &QPushButton::clicked, this, &ModelEvalPage::importSample);
    connect(ui->pushButton_mE_testOneSample, &QPushButton::clicked, this, &ModelEvalPage::testOneSample);

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
    cv::Mat imgSrc = cv::imread(choicedImagePath.c_str(), cv::IMREAD_COLOR);

    // 记录GroundTruth，包含四个坐标和类别信息
    vector<string> label_GT;
    vector<vector<cv::Point>> points_GT;
    string labelPath = choicedDatasetPATH+"/labelTxt/"+choicedImageFile.substr(0,choicedImageFile.size()-4)+".txt";
    dirTools->getGroundTruth(label_GT, points_GT, labelPath);
    // 绘制旋转框到图片上
    cv::drawContours(imgSrc, points_GT, -1, cv::Scalar(16, 124, 16), 2);
    // 绘制类别标签到图片上
    for(size_t i = 0; i<label_GT.size(); i++){
        cv::putText(imgSrc, label_GT[i], points_GT[i][1], cv::FONT_HERSHEY_COMPLEX, 0.4, cv::Scalar(0, 204, 0), 1);
    }
    // 将图片显示到界面上
    QPixmap pixmap = CVS::cvMatToQPixmap(imgSrc);
    ui->label_mE_img->setPixmap(pixmap.scaled(QSize(700,700), Qt::KeepAspectRatio));
    ui->label_mE_imgName->setText(QString::fromStdString(choicedImageFile));

}


int ModelEvalPage::importSample(){

    // TODO
}


int ModelEvalPage::testOneSample(){
    // TODO
    if(!choicedModelName.isEmpty() && !choicedSamplePATH.isEmpty()){
        // 使用TorchServe进行预测
        std::vector<std::map<QString,QString>> predMapStr = torchServe->inferenceOne(
            choicedModelName.split(".mar")[0], 
            choicedModelType, 
            choicedSamplePATH
        );

        // 解析预测结果predMapStr
        std::vector<std::vector<cv::Point>> predPoints;
        std::vector<QString> predLabels;
        std::vector<float> predScores;

        // 旋转框检测模型
        if(choicedModelType == "RBOX_DET"){
            // std::vector<cv::RotatedRect> predRRects;
            for(size_t i = 0; i<predMapStr.size(); i++){
                predLabels.push_back(predMapStr[i]["class_name"]);
                predScores.push_back(predMapStr[i]["score"].toFloat());

                QStringList predCoordStr = predMapStr[i]["bbox"].remove('[').remove(']').split(',');
                cv::RotatedRect predRRect;
                if(predCoordStr[4].toFloat()<0){
                    predRRect = cv::RotatedRect(
                        cv::Point2f(predCoordStr[0].toFloat(), predCoordStr[1].toFloat()),
                        cv::Size2f(predCoordStr[2].toFloat(), predCoordStr[3].toFloat()),
                        (predCoordStr[4].toFloat())*180
                    );
                }
                else{
                    predRRect = cv::RotatedRect(
                        cv::Point2f(predCoordStr[0].toFloat(), predCoordStr[1].toFloat()),
                        cv::Size2f(predCoordStr[3].toFloat(), predCoordStr[2].toFloat()),
                        (predCoordStr[4].toFloat()-PI/2)*180
                    );
                }

                cv::Point2f predPoint[4];
                predRRect.points(predPoint);
                std::vector<cv::Point> predPointVec;
                for(size_t j = 0; j<4; j++){
                    // std::cout<<predPoint[j].x<<"  "<<predPoint[j].y<<endl;
                    // 对predPoint进行深拷贝，保存至predPointVec
                    predPointVec.push_back(cv::Point(predPoint[j]));
                }
                // std::cout<<std::endl;
               predPoints.push_back(predPointVec);
            }   
        }

       // 绘制预测结果到图片上
       cv::Mat imgSrc = cv::imread(choicedSamplePATH.toStdString(), cv::IMREAD_COLOR);
       cv::drawContours(imgSrc, predPoints, -1, cv::Scalar(16, 124, 16), 2);
       // 绘制类别标签到图片上
       for(size_t i = 0; i<predLabels.size(); i++){
           cv::putText(imgSrc,
               predLabels[i].toStdString()+": "+std::to_string(predScores[i]).substr(0,5),
               predPoints[i][1],
               cv::FONT_HERSHEY_COMPLEX,
               0.4, cv::Scalar(0, 204, 0),
               1
           );
       }
       // 将图片显示到界面上
       QPixmap pixmap = CVS::cvMatToQPixmap(imgSrc);
       ui->label_mE_img->setPixmap(pixmap.scaled(QSize(700,700), Qt::KeepAspectRatio));
       ui->label_mE_imgName->setText(choicedSamplePATH.split("/").last());
    }
}
