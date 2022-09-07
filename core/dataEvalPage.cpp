#include "dataEvalPage.h"

// #include "lib/guiLogic/tools/convertTools.h"

#include <QMessageBox>
#include <QFileDialog>
#include <algorithm>
#include <cstdio>

using namespace std;
#define PI 3.1415926

DataEvalPage::DataEvalPage(Ui_MainWindow *main_ui,
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
    refreshGlobalInfo();
    connect(ui->pushButton_mE_importData, &QPushButton::clicked, this, &DataEvalPage::importData);
    connect(ui->pushButton_mE_testAll, &QPushButton::clicked, this, &DataEvalPage::testAll);

    uiResult.emplace("mAP50",ui->label_map50Num);
    uiResult.emplace("mPrec",ui->label_precNum);
    uiResult.emplace("mRecall",ui->label_recNum);
    uiResult.emplace("mcfar",ui->label_cfarNum);
    uiResult.emplace("gtAll",ui->label_gtNum);
    uiResult.emplace("preAll",ui->label_dtNum);
    uiResult.emplace("tp",ui->label_tpNum);
    uiResult.emplace("score",ui->label_scoreNum);

    resultMean.emplace("mAP50",0.0);
    resultMean.emplace("mPrec",0.0);
    resultMean.emplace("mRecall",0.0);
    resultMean.emplace("mcfar",0.0);
    resultMean.emplace("gtAll",0.0);
    resultMean.emplace("preAll",0.0);
    resultMean.emplace("tp",0.0);
    resultMean.emplace("score",0.0);

    
}


DataEvalPage::~DataEvalPage(){

}

int DataEvalPage::importData(){

}

int DataEvalPage::testAll(){
    result.clear();
    classType.clear();
    conMatrix.clear();
    clock_t start,finish;
    start = clock();
    float iouThresh = 0.5;
    vector<string> allSubDirs;
    // rec、prec测试样例
    // std::vector<float> rec = {0.0666, 0.1333,0.1333, 0.4, 0.4666};
    // std::vector<float> prec = {1., 0.6666, 0.6666, 0.4285, 0.3043};
    // float app = apCulcu(prec,rec);
    // printf("test_ap:%.4f\n",app);
    dirTools->getDirs(allSubDirs, choicedDatasetPATH);

    // printf("choicedDatasetPATH:",choicedDatasetPATH.c_str());
    // cout << "allSubDirs:" <<allSubDirs.back() << endl;
    vector<string> targetKeys = {"images","labelTxt"};
    for (auto &targetKey: targetKeys){
        if(!(std::find(allSubDirs.begin(), allSubDirs.end(), targetKey) != allSubDirs.end())){
            // 目标路径不存在目标文件夹
            QMessageBox::warning(NULL,"错误","该数据集路径下不存在"+QString::fromStdString(targetKey)+"文件夹！");
            return -1;
        }
    }
    // 获取当前数据集所有的样本类别
    std::string imageDir = choicedDatasetPATH+"/images";
    getClassName(imageDir);
    cout << "classNum:" << classType.size() << endl;
    for (int i = 0;i<classType.size();++i){
        cout << "classType:" << classType[i] << endl;
    }
    // 遍历所有的类别，分别计算指标
    // 获取图片文件夹下的所有图片文件名
    vector<string> imageFileNames;
    dirTools->getFiles(imageFileNames, ".png", choicedDatasetPATH+"/images");
    std::cout << "\n imageSize:" <<imageFileNames.size() << endl;
    int imageNum = 0;
    int predAll = 0;
    std::vector<int> gtNum;

    //存放全部的gt信息和pre信息，每个类别的不同，所以要放在类别循环内
    std::map<std::string, std::vector<pre_info>> preInfo;
    std::map<std::string, std::vector<gt_info>> gtInfo;

    // 为混淆矩阵做准备，每张图存好多框
    std::map<std::string,std::vector<gt_info_cm>> gtInfoMatrix;
    std::map<std::string,std::vector<pre_info_cm>> preInfoMatrix;

    // 混淆矩阵有背景，所以类别要多一个
    std::vector<std::string> matrixType(classType);
    matrixType.push_back("background");

    // 混淆矩阵初始化,+1是因为有背景
    for (size_t i = 0; i < matrixType.size(); i++)
    {
        for (size_t j = 0; j < matrixType.size(); j++)
        {
            conMatrix[matrixType[i]][matrixType[j]] = 0.0;
        }
    }
    
    // 第一层存放pre类别初始化，gtNum初始化
    for (size_t i = 0; i < classType.size(); i++)
    {
        std::vector<pre_info> preTmp;
        std::vector<gt_info> gtTmp;
        preInfo.emplace(classType[i],preTmp);
        gtInfo.emplace(classType[i],gtTmp);
        gtNum.push_back(0);
    }
    
    // 遍历图片名字，得到真实框坐标，生成curl命令
    for (auto &imageFileName:imageFileNames){
        //得到当前选取的图片路径
        imageNum = imageNum + 1;
        // std::cout << "imagefileName:" <<imageFileName<<endl;
        string choicedImagePath = choicedDatasetPATH+"/images/"+imageFileName;
        this->choicedSamplePATH = QString::fromStdString(choicedImagePath);
        // 读取GroundTruth，包含四个坐标和类别信息
        string labelPath = choicedDatasetPATH+"/labelTxt/"+imageFileName.substr(0,imageFileName.size()-4)+".txt";


        std::vector<std::vector<cv::Point>> points_GT;      // 存放真实框坐标
        std::vector<string>  label_GT;      //存放真实框类别标签
        // 获取当前图片的标注信息
        dirTools->getGroundTruth(label_GT,points_GT, labelPath);
        // 当前图片名字
        std::string localFileName = imageFileName.substr(0,imageFileName.size()-4);
        // 获取当前图片的真实框信息，存在对应类别里
        for (size_t i = 0; i < classType.size(); i++)
        {
            gt_info gtTemp;
            gtTemp.imgName = localFileName;
            //遍历单张图片里的所有真实框
            for (size_t j = 0; j < label_GT.size(); j++)
            {
                if(classType[i] == label_GT[j]){
                    gtTemp.gtRect.push_back(cv::minAreaRect(cv::Mat(points_GT[j])));
                    gtTemp.det.push_back(0);
                    // 计算真实框
                    gtNum[i] = gtNum[i] + 1;
                }
            }
            gtInfo[classType[i]].push_back(gtTemp);
        }

        // 混淆矩阵真实框的存储
        for (size_t i = 0; i < label_GT.size(); i++)
        {
            gt_info_cm matrixGtTmp;
            matrixGtTmp.className = label_GT[i];
            matrixGtTmp.gtRect = cv::minAreaRect(cv::Mat(points_GT[i]));
            gtInfoMatrix[localFileName].push_back(matrixGtTmp);
        }

        //预测当前图片并且将预测框存到对应的类别中
        if(!choicedModelName.isEmpty() && !choicedSamplePATH.isEmpty()){
            // 使用TorchServe进行预测
            std::vector<std::map<QString,QString>> predMapStr = torchServe->inferenceOne(
                choicedModelName.split(".mar")[0], 
                choicedModelType, 
                choicedSamplePATH
            );
            predAll = predAll + predMapStr.size();
            // 解析预测结果predMapStr
            if(choicedModelType == "RBOX_DET"){
                // 旋转框检测模型，map指标计算预测框存储
                for (size_t i = 0; i < classType.size(); i++)
                {
                    for(size_t j = 0; j < predMapStr.size(); j++){
                        if (predMapStr[j]["class_name"].toStdString() == classType[i]){
                            QStringList predCoordStr = predMapStr[j]["bbox"].remove('[').remove(']').split(',');
                            pre_info preTemp;
                            preTemp.imgName = localFileName;
                            preTemp.preRect.center.x = predCoordStr[0].toFloat();
                            preTemp.preRect.center.y = predCoordStr[1].toFloat();
                            preTemp.preRect.size.width = predCoordStr[2].toFloat();
                            preTemp.preRect.size.height = predCoordStr[3].toFloat();
                            preTemp.preRect.angle = predCoordStr[4].toFloat() * 180 / PI;
                            preTemp.score = predMapStr[j]["score"].toFloat();
                            preInfo[classType[i]].push_back(preTemp);
                        }
                    }
                }
                // 混淆矩阵预测框存储
                for (size_t i = 0; i < predMapStr.size(); i++)
                {
                    QStringList predCoordStr = predMapStr[i]["bbox"].remove('[').remove(']').split(',');
                    pre_info_cm matrixPreTmp;
                    matrixPreTmp.className = predMapStr[i]["class_name"].toStdString();
                    matrixPreTmp.preRect.center.x = predCoordStr[0].toFloat();
                    matrixPreTmp.preRect.center.y = predCoordStr[1].toFloat();
                    matrixPreTmp.preRect.size.width = predCoordStr[2].toFloat();
                    matrixPreTmp.preRect.size.height = predCoordStr[3].toFloat();
                    matrixPreTmp.preRect.angle = predCoordStr[4].toFloat() * 180 / PI;
                    matrixPreTmp.score = predMapStr[i]["score"].toFloat();
                    preInfoMatrix[localFileName].push_back(matrixPreTmp);
                }
                

            }
            else if(choicedModelType == "XXX"){
                // 正框检测模型
                // TODO
            }
        }
        // if (imageNum == 100) {
        //     break;
        // }
    }
    
    // 计算ap
    for (size_t cls = 0; cls < classType.size(); cls++)
    {
        // preInfo按照score排序
        sort(preInfo[classType[cls]].begin(),preInfo[classType[cls]].end());
        // tp和fp容器初始化为0
        std::vector<float> tp(preInfo[classType[cls]].size(),0.0);
        std::vector<float> fp(preInfo[classType[cls]].size(),0.0);
        std::vector<float> precision(preInfo[classType[cls]].size(),0.0);
        std::vector<float> recall(preInfo[classType[cls]].size(),0.0);
        float score = 0.0;
        // 计算tpfp
        tpfp(preInfo[classType[cls]],gtInfo[classType[cls]],tp,fp,iouThresh);
        // cumsum累加实现
        for(size_t i = 1;i<fp.size();i++){
            fp[i]=fp[i-1]+fp[i];
            tp[i]=tp[i-1]+tp[i];
            score = preInfo[classType[cls]][i].score + score;
        }
        // recall和precision计算
        for (size_t i = 0; i < precision.size(); i++)
        {
            if(tp[i]+fp[i] == 0.0){
                precision[i] = 0.0;
            }else{
                precision[i] = tp[i] / (tp[i]+fp[i]);
            }
            if(gtNum[cls] == 0){
                recall[i] = 0;
            }else{
                recall[i] = tp[i] / gtNum[cls];
            }
        }
        float ap = apCulcu(precision,recall);
        float prec = tp.back() / (tp.back()+fp.back());
        float rec = tp.back() / gtNum[cls];
        float cfar = 1 - prec;
        score = score / tp.size();
        result_ resultTmp = {classType[cls],gtNum[cls],int(preInfo[classType[cls]].size()),fp.back(),tp.back(),ap,rec,prec,cfar,score};
        result.push_back(resultTmp);
        

    }
    // 程序运行时间计算
    finish = clock();
    double allTime = double(finish-start) / CLOCKS_PER_SEC;
    double perTime = allTime / imageNum;
    cout << "codeAllTime:" << allTime << " perTime:" << perTime << endl;
    // 总体指标计算

    for (size_t i = 0; i < classType.size(); i++)
    {
        // cout << result[i].className << " ap:" << result[i].ap << endl;
        // cout << "gtNum:" << result[i].gtNUm << " detNum:" << result[i].detNUm << endl;
        // cout << "fp:" << result[i].fp << " tp:" << result[i].tp << endl;
        // cout << "recall:" << result[i].recall << " precision:" << result[i].precision << endl;
        // cout << "cfar:" << result[i].cfar << endl;
        resultMean["mAP50"] = resultMean["mAP50"] + result[i].ap;
        resultMean["mPrec"] = resultMean["mPrec"] + result[i].precision;
        resultMean["mRecall"] = resultMean["mRecall"] + result[i].recall;
        resultMean["mcfar"] = resultMean["mcfar"] + result[i].cfar;
        resultMean["gtAll"] = resultMean["gtAll"] + float(result[i].gtNUm);
        resultMean["preAll"] = resultMean["preAll"] + float(result[i].detNUm);
        resultMean["tp"] = resultMean["tp"] + float(result[i].tp);
        resultMean["score"] = resultMean["score"] + float(result[i].score);
    }
    resultMean["mAP50"] = resultMean["mAP50"] / classType.size();
    resultMean["mPrec"] = resultMean["mPrec"] / classType.size();
    resultMean["mRecall"] = resultMean["mRecall"] / classType.size();
    resultMean["mcfar"] = resultMean["mcfar"] / classType.size();
    resultMean["score"] = resultMean["score"] / classType.size();
    updateUiResult();

    confusionMatrix(gtInfoMatrix,preInfoMatrix,matrixType,conMatrix,iouThresh,0.3);
    plotConMatrix(conMatrix,matrixType);
    histogram(result,resultMean);


}

void DataEvalPage::updateUiResult(){
    // for(auto &subResult: this->uiResult){
    //     subResult.second->setText(QString("%1").arg(resultMean[subResult.first]));
    // }
    
    for(auto subResult: this->uiResult){
        std::string type = subResult.first;
        if(type == "gtAll" || type == "preAll" || type == "tp"){
            subResult.second->setText(QString("%1").arg(resultMean[subResult.first]));
        }else{
            subResult.second->setText(QString::number(resultMean[subResult.first]*100,'f',2).append("%"));
        }
        
    }
    
}

void DataEvalPage::refreshGlobalInfo(){
    // 基本信息更新
    this->choicedModelName = QString::fromStdString(modelInfo->selectedName);
    this->choicedModelType = QString::fromStdString(modelInfo->selectedType);
    
    ui->label_mE_dataset_2->setText(QString::fromStdString(datasetInfo->selectedName));
    ui->label_mE_model_2->setText(choicedModelName);
    ui->label_mE_batch_2->setText(QString::fromStdString(modelInfo->getAttri(modelInfo->selectedType, modelInfo->selectedName, "batch")));
    this->choicedDatasetPATH = datasetInfo->getAttri(datasetInfo->selectedType,datasetInfo->selectedName,"PATH");

}

void DataEvalPage::calcuRectangle(cv::Point centerXY, cv::Size wh, float angle, std::vector<cv::Point> &fourPoints){
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


float DataEvalPage::mmdetIOUcalcu(cv::RotatedRect rect1,cv::RotatedRect rect2){

    if (rect1.center.x > rect2.center.x+rect2.size.width) { return 0.0; }
    if (rect1.center.y > rect2.center.y+rect2.size.height) { return 0.0; }
    if (rect1.center.x+rect1.size.width < rect2.center.x) { return 0.0; }
    if (rect1.center.y+rect1.size.height < rect2.center.y) { return 0.0; }
    float area1 = rect1.size.width * rect1.size.height;
    float area2 = rect2.size.width * rect2.size.height;
    float colInt = min(rect1.center.x + rect1.size.width,rect2.center.x + rect2.size.width) - max(rect1.center.x,rect2.center.x);
    float rowInt = min(rect1.center.y + rect1.size.height,rect2.center.y + rect2.size.height) - max(rect1.center.y,rect2.center.y);
    float intersection = colInt * rowInt;       //交集
    float iou = intersection / (area1 + area2 -intersection);
    return iou;
}


float DataEvalPage::rotateIOUcv(cv::RotatedRect rect1,cv::RotatedRect rect2){
    float areaRect1 = rect1.size.width * rect1.size.height;
    float areaRect2 = rect2.size.width * rect2.size.height;
    vector<cv::Point2f> vertices;
    int intersectionType = cv::rotatedRectangleIntersection(rect1, rect2, vertices);
    if (vertices.size()==0)
        return 0.0;
    else{
        vector<cv::Point2f> order_pts;
        // 找到交集（交集的区域），对轮廓的各个点进行排序
        cv::convexHull(cv::Mat(vertices), order_pts, true);
        double area = cv::contourArea(order_pts);
        float inner = (float) (area / (areaRect1 + areaRect2 - area + 0.0001));
        return inner;
    }

}

void DataEvalPage::getClassName(std::string dirPath){
    vector<string> imageFileNames;
    vector<string> class_GT;
    std::vector<string> gtLabel;
    std::vector<std::vector<cv::Point>> gtPoints;
    dirTools->getFiles(imageFileNames, ".png", dirPath);
    std::cout << "\n imageSize:" <<imageFileNames.size() << endl;
    std::int8_t num = 0;
    
    // 遍历图片名字
    for (auto &imageFileName:imageFileNames){
        //得到当前选取的图片路径d
        // std::cout << "imagefileName:" <<imageFileName<<endl;
        string choicedImagePath = choicedDatasetPATH+"/images/"+imageFileName;
        this->choicedSamplePATH = QString::fromStdString(choicedImagePath);
        // 读取GroundTruth，包含四个坐标和类别信息
        string labelPath = choicedDatasetPATH+"/labelTxt/"+imageFileName.substr(0,imageFileName.size()-4)+".txt";
        dirTools->getGroundTruth(gtLabel, gtPoints, labelPath);
        for (int i = 0;i<gtLabel.size();++i){
            if (find(classType.begin(),classType.end(),gtLabel[i])!=classType.end()){
                continue;
            }
            else{
                classType.push_back(gtLabel[i]);
            }
        }
    }
}


void DataEvalPage::tpfp(std::vector<pre_info> preInfo,
    std::vector<gt_info> gtInfo,
    std::vector<float> &tp,
    std::vector<float> &fp,
    float iouThresh){
    int num =0;
    // 计算预测框和该图所有真实框的IOU，取最大值
    for (size_t i = 0; i < preInfo.size(); i++)
    {
        float iouMax = 0.0;
        std::vector<float> IOU;
        for (size_t j = 0; j < gtInfo.size(); j++)
        {
            if( preInfo[i].imgName == gtInfo[j].imgName){
                for(size_t k = 0; k < gtInfo[j].gtRect.size();k++){
                    IOU.push_back(rotateIOUcv(preInfo[i].preRect,gtInfo[j].gtRect[k]));
                }
                for (size_t m = 0; m < IOU.size(); m++)
                {
                    if(iouMax<IOU[m]){
                        iouMax = IOU[m];
                    }
                }
                // 最大IOU对应索引
                // int iouMaxIdx = (int)(std::distance(IOU.begin(), iouMax));
                int iouMaxIdx = (int)(std::max_element(IOU.begin(), IOU.end()) - (IOU.begin()));
                // 计算tp 和fp个数
                if(iouMax > iouThresh){
                    if(gtInfo[j].det[iouMaxIdx] == 0){
                        tp[i] = 1.0;
                        gtInfo[j].det[iouMaxIdx] = 1;
                    }
                    else{
                        fp[i] = 1.0;
                    }
                }
                else{
                    fp[i] = 1.0;
                }
                num = num + 1;
            }
            // 得到预测框与当前图中真实框的最大IOU
            // auto iouMaxit = max_element(IOU.begin(), IOU.end());
            // auto iouMax = *iouMaxit;     //这里莫名其妙不能这样用，不然系统会崩溃，只能自己写一个
            // auto iouMax = *max_element(IOU.begin(), IOU.end());
        }
    }
    cout << "usedDt:" << num << endl;
}

float DataEvalPage::apCulcu(std::vector<float> precision,std::vector<float> recall){
    precision.push_back(0.0);
    recall.push_back(1.0);
    //初始位置补0
    recall.insert(recall.begin(),0.0);
    precision.insert(precision.begin(),0.0);
    //存放recall中不重复的序号
    std::vector<size_t> index;      
    float apSum = 0.0;
    // precision两两取最大值
    for (size_t i = precision.size()-1; i > 0; i--)
    {
        precision[i-1] = max(precision[i],precision[i-1]);
    }
    

    // 寻找recall中值变化的序号
    for (size_t i = 1; i < recall.size()-1; i++)
    {
        if(recall[i] != recall[i-1]){
            index.push_back(i);
        }else{
            continue;
        }
    }
    //求AP
    for (size_t i = 0; i < index.size(); i++)
    {
        apSum = apSum + ((recall[index[i]]-recall[index[i]-1]) * precision[index[i]]);
    }
    return apSum;
}
// void DataEvalPage::

// 计算混淆矩阵
void DataEvalPage::confusionMatrix(
    std::map<std::string,std::vector<gt_info_cm>> gtInfo,
    std::map<std::string,std::vector<pre_info_cm>> preInfo,
    std::vector<std::string> matrixType,
    std::map<std::string,std::map<std::string, float>> &Matrix,
    float iouThresh,float scoreThresh){

    //获取混淆矩阵维度
    for (auto perImgPre:preInfo)
    {
        std::vector<int> true_positives(gtInfo[perImgPre.first].size(),0.0);
        std::map<int,std::map<int,float>> ious;
        for (size_t i = 0; i < perImgPre.second.size(); i++)
        {
            for (size_t j = 0; j < gtInfo[perImgPre.first].size(); j++)
            {
                ious[i][j] = rotateIOUcv(perImgPre.second[i].preRect,gtInfo[perImgPre.first][j].gtRect);
                // ious.push_back(rotateIOUcv(perImgPre.second[i].preRect,gtInfo[perImgPre.first][j].gtRect));
            }
        }
        for (size_t i = 0; i < perImgPre.second.size(); i++)
        {
            int det_match = 0;
            if(perImgPre.second[i].score > scoreThresh){
                for (size_t j = 0; j < gtInfo[perImgPre.first].size(); j++)
                {
                    if(ious[i][j] >= iouThresh){
                        det_match = det_match + 1;
                        if(perImgPre.second[i].className == gtInfo[perImgPre.first][j].className){
                            true_positives[j] += 1;
                        }
                        Matrix[gtInfo[perImgPre.first][j].className][perImgPre.second[i].className] += 1.0;
                    }
                }
                if(det_match == 0){
                    Matrix[matrixType.back()][perImgPre.second[i].className] += 1.0;
                }
            }

        }
        for (size_t i = 0; i < true_positives.size(); i++)
        {
            if(true_positives[i] == 0){
                Matrix[gtInfo[perImgPre.first][i].className][matrixType.back()] += 1.0;
            }
        }
    }
}




// 绘制混淆矩阵
void DataEvalPage::plotConMatrix(std::map<std::string,std::map<std::string, 
    float>> conMatrix,std::vector<std::string> matrixType){
    ui->qTable->setWindowTitle("Confusion Matrix");
    ui->qTable->setColumnCount(matrixType.size());
    ui->qTable->setRowCount(matrixType.size());
    // 行列标题设置
    QStringList h_Header;
    QStringList v_Header;
    for (size_t i = 0; i < matrixType.size(); i++)
    {
        h_Header.append(QString::fromStdString(matrixType[i]));
        v_Header.append(QString::fromStdString(matrixType[i]));
    }
    ui->qTable->setHorizontalHeaderLabels(h_Header);
    ui->qTable->setVerticalHeaderLabels(v_Header);
    ui->qTable->setStyleSheet("QHeaderView::section{background:white;}");
    ui->qTable->horizontalHeader()->setVisible(true);
    ui->qTable->verticalHeader()->setVisible(true);
    //窗口自适应
    // ui->qTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // ui->qTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 不可以编辑
    ui->qTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 根据内容设置行列长度
    // ui->qTable->resizeColumnsToContents();
    // ui->qTable->resizeRowsToContents();

    std::vector<float> iSum;
    for (size_t i = 0; i < matrixType.size(); i++)
    {
        float iSumTmp = 0;
        for (size_t j = 0; j < matrixType.size(); j++)
        {
            iSumTmp = iSumTmp + conMatrix[matrixType[i]][matrixType[j]];
        }
        iSum.push_back(iSumTmp);
    }
    // 单元格赋值
    for (size_t i = 0; i < matrixType.size(); i++)
    {
        for (size_t j = 0; j < matrixType.size(); j++)
        {
            
            if(iSum[i]>0){
                float res = conMatrix[matrixType[i]][matrixType[j]] / iSum[i] *100;
                ui->qTable->setItem(i,j,new QTableWidgetItem(QString::number(res,'f',0).append("%")));
                ui->qTable->item(i,j)->setTextAlignment(Qt::AlignCenter);
            }else{
                ui->qTable->setItem(i,j,new QTableWidgetItem(QString::number(0).append("%")));
            }
        }
    }
    // 单元格细节美化
    for (size_t i = 0; i < matrixType.size(); i++)
    {
        for (size_t j = 0; j < matrixType.size(); j++)
        {
            // 文本居中
            ui->qTable->item(i,j)->setTextAlignment(Qt::AlignCenter);
        }
    }
}

void removeLayout(QLayout *layout){
    QLayoutItem *child;
    if (layout == nullptr)
        return;
    while ((child = layout->takeAt(0)) != nullptr){
        // child可能为QLayoutWidget、QLayout、QSpaceItem
        // QLayout、QSpaceItem直接删除、QLayoutWidget需删除内部widget和自身
        if (QWidget* widget = child->widget()){
            widget->setParent(nullptr);
            delete widget;
            widget = nullptr;
        }

        else if (QLayout* childLayout = child->layout())
            removeLayout(childLayout);

        delete child;
        child = nullptr;
    }
}


// 绘制柱状图
void DataEvalPage::histogram(std::vector<result_> result,std::map<std::string,float> resultMean){
    //创建条形组
    std::vector<float> ap50;
    std::vector<float> recall;
    std::vector<float> score;
    std::vector<std::string> chartType(classType);
    chartType.push_back("平均");
    for (size_t i = 0; i < classType.size(); i++)
    {
        ap50.push_back(result[i].ap);
        recall.push_back(result[i].recall);
        score.push_back(result[i].score);
    }
    // 加入总的值
    ap50.push_back(resultMean["mAP50"]);
    recall.push_back(resultMean["mRecall"]);
    score.push_back(resultMean["score"]);
    
    QChart *chart = new QChart;
    std::map<QString, vector<float>> mapnum;
    mapnum.insert(pair<QString, std::vector<float>>("AP50", ap50));  //后续可拓展
    mapnum.insert(pair<QString, std::vector<float>>("Recall", recall));
    mapnum.insert(pair<QString, std::vector<float>>("Score", score));

    QBarSeries *series = new QBarSeries();
    map<QString, vector<float>>::iterator it = mapnum.begin();
    //将数据读入
    while (it != mapnum.end()){
        QString tit = it->first;
        QBarSet *set = new QBarSet(tit);
        std::vector<float> vecnum = it->second;
        for (auto a : vecnum){
            // 百分号
            a = a * 100;
            // 保留两位
            a = ((float)((int)((a + 0.005) * 100 ))) / 100;
            *set << a;
        }
        series->append(set);
        it++;
    }
    // QString::number(resultMean[subResult.first],'f',2).append("%")
    series->setVisible(true);
    series->setLabelsVisible(true);
    // 横坐标参数
    QBarCategoryAxis *axis = new QBarCategoryAxis;
    for(int i = 0; i<chartType.size(); i++){
        axis->append(QString::fromStdString(chartType[i]));
    }
    QValueAxis *axisy = new QValueAxis;
    axisy->setRange(0,100);
    axisy->setTitleText("%");
    QFont chartLabel;
    chartLabel.setPixelSize(14);
    chart->addSeries(series);
    chart->setTitle("各类别具体参数图");
    chart->setTitleFont(chartLabel);

    chart->setAxisX(axis, series);
    chart->setAxisY(axisy, series);
    chart->legend()->setVisible(true);

    QChartView *view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    removeLayout(ui->horizontalLayout_histogram);
    ui->horizontalLayout_histogram->addWidget(view);
}
