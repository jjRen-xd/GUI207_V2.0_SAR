#include "testThread.h"
using namespace std;
#define PI 3.1415926
TestThread::TestThread(DatasetInfo *globalDatasetInfo,
                       EvaluationIndex *evalBack,
                       std::string *choicedDatasetPATH,
                       QString *choicedModelName,
                       QString *choicedModelType) : datasetInfo(globalDatasetInfo),
                                                    eval(evalBack),
                                                    choicedDatasetPATH(choicedDatasetPATH),
                                                    choicedModelName(choicedModelName),
                                                    choicedModelType(choicedModelType)
{
    qRegisterMetaType<std::vector<result_>>("std::vector<result_>");
    qRegisterMetaType<std::vector<std::string>>();
    qRegisterMetaType<CMmap>("CMmap");
    MaskApi *maskiou;
    stopped = false;
    errorTag = false;
}

TestThread::~TestThread()
{
}

void TestThread::stop()
{
    stopped = true;
}

void TestThread::run()
{
    // stopped = false;
    //qDebug() << "线程开始运行";
    // while (!stopped){
    float iouThresh = 0.5;
    float scoreThresh = 0.3;
    // 全局变量清空
    classType.clear();
    conMatrix.clear();
    result.clear();

    vector<string> allSubDirs;
    dirTools->getDirs(allSubDirs, *choicedDatasetPATH);
    vector<string> targetKeys = {"images", "labelTxt"};

    for (auto &targetKey : targetKeys)
    {
        if (!(std::find(allSubDirs.begin(), allSubDirs.end(), targetKey) != allSubDirs.end()))
        {
            // 目标路径不存在目标文件夹
            QMessageBox::warning(NULL, "错误", "该数据集路径下不存在" + QString::fromStdString(targetKey) + "文件夹！");
            // return -1;
        }
    }

    // 直接从类别文件夹读取类别
    vector<string> classNames;
    if (dirTools->getFiles(classNames, ".jpg", *choicedDatasetPATH + "/classImages/"))
    {
        for (size_t i = 0; i < classNames.size(); i++)
        {
            auto classTmp = QString::fromStdString(classNames[i]).split(".")[0];
            classType.push_back(classTmp.toStdString());
        }
    }
    for (size_t i = 0; i < classType.size(); i++)
    {
        std::cout << "classType:" << classType[i] << endl;
    }
    std::cout << "classSize:" << classType.size() << endl;

    // 遍历所有的类别，分别计算指标
    // 获取图片文件夹下的所有图片文件名
    vector<string> imageFileNames;

    if (0 == datasetInfo->selectedType.compare("BBOX"))
    {
        dirTools->getFiles(imageFileNames, ".jpg", *choicedDatasetPATH + "/images");
        std::cout << "imageSize:" << imageFileNames.size() << std::endl;
    }
    else
    {
        dirTools->getFiles(imageFileNames, ".png", *choicedDatasetPATH + "/images");
        std::cout << "imageSize:" << imageFileNames.size() << std::endl;
    }
    int imageNum = 0;
    int predAll = 0;
    std::vector<int> gtNum;

    // 根据正框还是旋转框做出判断

    // 存放全部的gt信息和pre信息，每个类别的不同，所以要放在类别循环内
    std::map<std::string, std::vector<pre_info>> preInfo;
    std::map<std::string, std::vector<gt_info>> gtInfo;

    // 为混淆矩阵做准备，每张图存好多框
    std::map<std::string, std::vector<gt_info_cm>> gtInfoMatrix;
    std::map<std::string, std::vector<pre_info_cm>> preInfoMatrix;

    // 混淆矩阵有背景，所以类别要多一个
    std::vector<std::string> matrixType(classType);
    matrixType.push_back("background");
    qDebug() << "matrixSizd:" << matrixType.size();
    // 混淆矩阵初始化,+1是因为有背景""
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
        preInfo.emplace(classType[i], preTmp);
        gtInfo.emplace(classType[i], gtTmp);
        gtNum.push_back(0);
    }

    size_t error_model = 0;
    // 遍历图片名字，得到真实框坐标，生成curl命令
    for (auto &imageFileName : imageFileNames)
    {
        // 用于计算连续空预测的图片个数，超过十个就判断为模型不行

        // 收到外部的终止信号，则退出这个图片循环
        if (stopped)
        {
            break;
        }
        // 得到当前选取的图片路径
        imageNum = imageNum + 1;
        string choicedImagePath = *choicedDatasetPATH + "/images/" + imageFileName;
        this->choicedSamplePATH = QString::fromStdString(choicedImagePath);
        // 旋转框预测
        if (*choicedModelType == "RBOX_DET")
        {
            bboxTag = false;
            // 读取GroundTruth，包含四个坐标和类别信息
            string labelPath = *choicedDatasetPATH + "/labelTxt/" + imageFileName.substr(0, imageFileName.size() - 4) + ".txt";
            std::vector<std::vector<cv::Point>> points_GT; // 存放真实框坐标
            std::vector<string> label_GT;                  // 存放真实框类别标签
            // 获取当前图片的标注信息
            dirTools->getGroundTruth(label_GT, points_GT, labelPath);
            // 当前图片名字
            std::string localFileName = imageFileName.substr(0, imageFileName.size() - 4);
            // 获取当前图片的真实框信息，存在对应类别里
            for (size_t i = 0; i < classType.size(); i++)
            {
                gt_info gtTemp;
                gtTemp.imgName = localFileName;
                // 遍历单张图片里的所有真实框
                for (size_t j = 0; j < label_GT.size(); j++)
                {
                    if (classType[i] == label_GT[j])
                    {
                        gtTemp.gtRect.push_back(cv::minAreaRect(cv::Mat(points_GT[j])));
                        gtTemp.det.push_back(0);
                        // 真实框数量计算
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
            // 预测当前图片并且将预测框存到对应的类别中
            if (!choicedModelName->isEmpty() && !choicedSamplePATH.isEmpty())
            {
                // 使用TorchServe进行预测
                std::vector<std::map<QString, QString>> predMapStr = eval->torchServe->inferenceOne(
                    choicedModelName->split(".mar")[0],
                    *choicedModelType,
                    choicedSamplePATH);
                if (predMapStr.empty() == 0)
                {
                    predAll = predAll + predMapStr.size();
                    // qDebug() << "predAll:" <<predAll;
                    // 解析预测结果predMapStr
                    // 旋转框检测模型，map指标计算预测框存储
                    for (size_t i = 0; i < classType.size(); i++)
                    {
                        for (size_t j = 0; j < predMapStr.size(); j++)
                        {
                            if (predMapStr[j]["class_name"].toStdString() == classType[i])
                            {
                                QStringList predCoordStr = predMapStr[j]["bbox"].remove('[').remove(']').split(',');
                                pre_info preTemp;
                                preTemp.imgName = localFileName;
                                preTemp.preRect.center.x = predCoordStr[0].toFloat();
                                preTemp.preRect.center.y = predCoordStr[1].toFloat();
                                preTemp.preRect.size.width = predCoordStr[2].toFloat();
                                preTemp.preRect.size.height = predCoordStr[3].toFloat();
                                // rad转angle
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
                else
                {
                    error_model += 1;
                }
            }
        }
        else
        {
            // 如果是正框，读取xml标注
            bboxTag = true;
            // 读取GroundTruth,正框xy极大极小值
            string labelPath = *choicedDatasetPATH + "/labelTxt/" + imageFileName.substr(0, imageFileName.size() - 4) + ".xml";
            std::vector<std::vector<cv::Point>> points_GT; // 存放真实框坐标
            std::vector<string> label_GT;                  // 存放真实框类别标签
            std::vector<std::vector<std::double_t>> bboxGT;
            // 获取当前图片的标注信息
            dirTools->getGtXML(label_GT, points_GT, bboxGT, labelPath);
            // 当前图片名字
            std::string localFileName = imageFileName.substr(0, imageFileName.size() - 4);

            // 获取当前图片的真实框信息，存在对应类别里
            for (size_t i = 0; i < classType.size(); i++)
            {
                gt_info gtTemp;
                gtTemp.imgName = localFileName;
                // 遍历单张图片里的所有真实框
                for (size_t j = 0; j < label_GT.size(); j++)
                {
                    if (classType[i] == label_GT[j])
                    {
                        // gtTemp.gtRect.push_back(cv::minAreaRect(cv::Mat(points_GT[j])));
                        gtTemp.gtBbox.push_back(bboxGT[j]);
                        gtTemp.det.push_back(0);
                        // 真实框数量计算
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
                // matrixGtTmp.gtRect = cv::minAreaRect(cv::Mat(points_GT[i]));
                matrixGtTmp.gtBbox = bboxGT[i];
                gtInfoMatrix[localFileName].push_back(matrixGtTmp);
            }

            // 预测当前图片并且将预测框存到对应的类别中
            if (!choicedModelName->isEmpty() && !choicedSamplePATH.isEmpty())
            {
                // 使用TorchServe进行预测
                std::vector<std::map<QString, QString>> predMapStr = eval->torchServe->inferenceOne(
                    choicedModelName->split(".mar")[0],
                    *choicedModelType,
                    choicedSamplePATH);
                if (predMapStr.empty() == 0)
                {
                    predAll = predAll + predMapStr.size();
                    // 解析预测结果predMapStr
                    // 旋转框检测模型，map指标计算预测框存储

                    // 预测框获取和存储
                    for (size_t i = 0; i < classType.size(); i++)
                    {
                        for (size_t j = 0; j < predMapStr.size(); j++)
                        {
                            if (predMapStr[j]["class_name"].toStdString() == classType[i])
                            {
                                // 三点构造正框矩形，预测框出来的是左下角坐标和宽高

                                QStringList predCoordStr = predMapStr[j]["bbox"].remove('[').remove(']').split(',');
                                pre_info preTemp;
                                cv::Point points[4];
                                double xmin = predCoordStr[0].toDouble();
                                double ymin = predCoordStr[1].toDouble();
                                double xmax = predCoordStr[2].toDouble();
                                double ymax = predCoordStr[3].toDouble();

                                double w = xmax - xmin;
                                double h = ymax - ymin;

                                preTemp.preBbox = {xmin, ymin, w, h};
                                preTemp.imgName = localFileName;
                                preTemp.score = predMapStr[j]["score"].toDouble();
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
                        cv::Point points[4];
                        double xmin = predCoordStr[0].toDouble();
                        double ymin = predCoordStr[1].toDouble();
                        double xmax = predCoordStr[2].toDouble();
                        double ymax = predCoordStr[3].toDouble();

                        double w = xmax - xmin;
                        double h = ymax - ymin;
                        matrixPreTmp.preBbox = {xmin, ymin, w, h};
                        matrixPreTmp.score = predMapStr[i]["score"].toDouble();
                        preInfoMatrix[localFileName].push_back(matrixPreTmp);
                    }
                }
                else
                {
                    error_model += 1;
                }
            }
            // if(imageNum == 50){
            //     break;
            // }
        }
        if (imageNum < 10 && error_model > 5)
        {
            // 停止线程和错误标志
            stopped = true;
            errorTag = true;
            break;
        }
    }
    // 如果收到终止信号
    if (stopped)
    {
        // 并且收到错误信号，说明是错误导致终止
        if (errorTag)
        {
            // 发送异常中止提醒
            emit errorStop(true);
            msleep(1);
            stopped = false;
            errorTag = false;
        }
        // 否则就是正常终止
        else{
            // stopped设置为false用于下次启动线程
            stopped = false;
        }
    }
    else
    {
        for (size_t cls = 0; cls < classType.size(); cls++)
        {
            cout << "GtNum = " << gtNum[cls] << endl;
        }
        // cout << "GtNum = " << gtNum[0] << endl;
        cout << "DtNum = " << predAll << endl;

        // 计算ap

        for (size_t cls = 0; cls < classType.size(); cls++)
        {
            // preInfo按照score排序
            sort(preInfo[classType[cls]].begin(), preInfo[classType[cls]].end());
            // 这里跑出来是正确的
            // cout << "DtScoreFirst:" << preInfo[classType[0]][0].score << endl;
            // cout << "DtScoreEnd:" << preInfo[classType[0]][-1].score << endl;
            // for (size_t i =0;i < preInfo[classType[cls]].size();i++){
            //     cout << "DtScore:" << preInfo[classType[cls]][i].score << endl;
            // }
            // tp和fp容器初始化为0
            std::vector<float> tp(preInfo[classType[cls]].size(), 0.0);
            std::vector<float> fp(preInfo[classType[cls]].size(), 0.0);
            std::vector<double> precision(preInfo[classType[cls]].size(), 0.0);
            std::vector<double> recall(preInfo[classType[cls]].size(), 0.0);
            std::vector<double> scoreVector(preInfo[classType[cls]].size(), 0.0);
            double score = 0.0;
            // 计算tpfp
            eval->tpfp(bboxTag, preInfo[classType[cls]], gtInfo[classType[cls]], tp, fp, iouThresh);
            // cumsum累加实现,到这为止都是正确的
            for (size_t i = 1; i < fp.size(); i++)
            {
                fp[i] = fp[i - 1] + fp[i];
                tp[i] = tp[i - 1] + tp[i];
                // cout << "fp_" << i-1 << ":" << fp[i-1] << endl;
                scoreVector[i] = preInfo[classType[cls]][i].score;
                score = preInfo[classType[cls]][i].score + score;
            }
            // recall和precision计算,到这里也都是正确的对得上
            for (size_t i = 0; i < precision.size(); i++)
            {
                if (tp[i] + fp[i] == 0.0)
                {
                    precision[i] = 0.0;
                }
                else
                {
                    precision[i] = tp[i] / (tp[i] + fp[i]);
                    // cout << "precision_" << i << ":" << precision[i] << endl;
                }
                if (gtNum[cls] == 0)
                {
                    recall[i] = 0;
                }
                else
                {
                    recall[i] = tp[i] / gtNum[cls];
                    // cout << "recall_" << i << ":" << recall[i] << endl;
                }
            }
            std::vector<double> apResult = eval->apCulcu(precision, recall, scoreVector);
            double prec = 0;
            double cfar = 0;
            double rec = 0;
            if ((tp.back() + fp.back()) > 0)
            {
                prec = tp.back() / (tp.back() + fp.back());
            }
            cfar = 1 - prec;
            if (gtNum[cls] > 0)
            {
                rec = recall.back();
            }
            if (tp.size() > 0)
            {
                score = score / tp.size();
            }
            double ap = apResult[0];
            score = apResult[1];
            result_ resultTmp = {classType[cls], gtNum[cls], int(preInfo[classType[cls]].size()), fp.back(), tp.back(), ap, rec, prec, cfar, score};
            result.push_back(resultTmp);
        }

        eval->confusionMatrix(bboxTag, gtInfoMatrix, preInfoMatrix, matrixType, conMatrix, iouThresh, scoreThresh);
        
        if (bboxTag)
        {
//             for (size_t i = 0; i < classType.size(); i++)
//             {
//                 result[i].cfar = conMatrix[matrixType[1]][matrixType[0]] / (conMatrix[matrixType[1]][matrixType[0]] + conMatrix[matrixType[0]][matrixType[0]]);
//             }
             double conPrec = 0.0;
             for (size_t i = 0; i < classType.size(); i++)
             {
                 // 求conMatrix[matrixType[i]]的和
                 double sum = 0.0;
                 for (size_t j = 0; j <= classType.size(); j++)
                 {
                     sum += conMatrix[matrixType[j]][matrixType[i]];
                 }
                 conPrec = conMatrix[matrixType[i]][matrixType[i]] / sum;
                 result[i].cfar = 1 - conPrec;
             }
//            double conPrec = 0.0;
//            for (size_t i = 0; i < classType.size(); i++)
//            {
//                // 求conMatrix[matrixType[i]]的和
//                double sum = 0.0;
//                for (size_t j = 0; j < classType.size(); j++)
//                {
//                    sum += conMatrix[matrixType[i]][matrixType[j]];
//                }
//                conPrec= conMatrix[matrixType[i]][matrixType[i]] / sum;
//                result[i].cfar = 1 - conPrec;
//            }
        }
        if (result.empty())
        {
            cout << "no result" << endl;
        }

        emit end(result, classType, conMatrix);
        stopped = false;
    }

}
