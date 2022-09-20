#include "evaluationIndex.h"

using namespace std;
#define PI 3.1415926

EvaluationIndex::EvaluationIndex(DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo, TorchServe *globalTorchServe):
    datasetInfo(globalDatasetInfo),
    modelInfo(globalModelInfo),
    torchServe(globalTorchServe)
{



}

EvaluationIndex::~EvaluationIndex(){
    
}

float EvaluationIndex::rotateIOUcv(cv::RotatedRect rect1,cv::RotatedRect rect2){
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

void EvaluationIndex::tpfp(std::vector<pre_info> preInfo,
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

float EvaluationIndex::apCulcu(std::vector<float> precision,std::vector<float> recall){
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
// void EvaluationIndex::

// 计算混淆矩阵
void EvaluationIndex::confusionMatrix(
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