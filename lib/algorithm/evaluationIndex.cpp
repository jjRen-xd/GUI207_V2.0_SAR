#include "evaluationIndex.h"

using namespace std;
#define PI 3.1415926

EvaluationIndex::EvaluationIndex(DatasetInfo *globalDatasetInfo, ModelInfo *globalModelInfo, TorchServe *globalTorchServe):
    datasetInfo(globalDatasetInfo),
    modelInfo(globalModelInfo),
    torchServe(globalTorchServe)
{
    MaskApi *maskiou;


}

EvaluationIndex::~EvaluationIndex(){
    
}

// 斜框iou计算
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

void EvaluationIndex::tpfp(bool bboxTag,
    std::vector<pre_info> preInfo,
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
                // 如果是正框，走微软iou计算
                if (bboxTag){
                    for(size_t k = 0; k < gtInfo[j].gtBbox.size();k++){
                        BB db,gb;
                        double iou = 0;
                        // 构建和预测框真实框一样大小的数组
                        double dt[preInfo[i].preBbox.size()];
                        double gt[gtInfo[j].gtBbox[k].size()];
                        // 把vector的真实框和预测框转化为数组
                        memcpy(dt,&preInfo[i].preBbox[0],preInfo[i].preBbox.size() * sizeof(preInfo[i].preBbox[0]));
                        memcpy(gt,&gtInfo[j].gtBbox[k][0],gtInfo[j].gtBbox[k].size() * sizeof(gtInfo[j].gtBbox[k][0]));
                        db = dt;
                        gb = gt;
                        // 预测框个数
                        siz m = 1;
                        // 真实框个数
                        siz n = 1;
                        byte2 iscrowd = 0;
                        maskiou->bbIou(db,gb,m,n,&iscrowd,&iou);
                        // if(preInfo[i].imgName == "000449"){
                        //     cout << "iou: " << iou << endl;
                        // }
                        IOU.push_back(iou);
                    }
                }
                else{
                    for(size_t k = 0; k < gtInfo[j].gtRect.size();k++){
                        IOU.push_back(rotateIOUcv(preInfo[i].preRect,gtInfo[j].gtRect[k])); 
                    }
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
                        // if(gtInfo[j].imgName == "001039"){
                        //     // cout << gtInfo[j].imgName << endl;
                        //     cout << iouMax << endl;
                        // }
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
        }
    }
    cout << "usedDt:" << num << endl;
}


// 在start_in和end_in之间等分num_in个
std::vector<double> EvaluationIndex::linspace(double start_in, double end_in, int num_in)
{

  std::vector<double> linspaced;

  double start = static_cast<double>(start_in);
  double end = static_cast<double>(end_in);
  double num = static_cast<double>(num_in);

  if (num == 0) { return linspaced; }
  if (num == 1) 
    {
      linspaced.push_back(start);
      return linspaced;
    }

  double delta = (end - start) / (num - 1);

  for(int i=0; i < num-1; ++i)
    {
      linspaced.push_back(start + delta * i);
    }
  linspaced.push_back(end); // I want to ensure that start and end
                            // are exactly the same as the input
  return linspaced;
}


std::vector<double> EvaluationIndex::apCulcu(std::vector<double> precision,std::vector<double> recall,std::vector<double> score){

    // 长度为101的用于插入的数组，1分成101份
    linSpaced = this->linspace(0.0,1.00,101);

    // 最后用于平均的长度为101的precision和scores数组
    std::vector<double> pr(101,0.0);
    std::vector<double> ss(101,0.0);

    // 用于存放插入序号的长度为101的数组
    std::vector<size_t> insertArray;

    //存放recall中不重复的序号
    std::vector<size_t> index;      
    double apSum = 0.0;
    double scoreSum = 0.0;
    // precision两两取最大值
    for (size_t i = precision.size()-1; i > 0; i--)
    {
        precision[i-1] = max(precision[i],precision[i-1]);
    }

    // 选择101个插入的序号
    for (size_t i = 0; i < linSpaced.size();i++){
        // 如果小于第一个recall值，则为0，如果大于最后一个值，则为最后一个数，其他则在中间
        if (linSpaced[i] < recall[0])
        {
            insertArray.push_back(0);
        }else if (linSpaced[i] > recall.back()){
            insertArray.push_back(recall.size());
        }else{
            for (size_t j = 1; j < recall.size();j++){
                if (linSpaced[i] <= recall[j] && linSpaced[i] > recall[j-1]){
                    insertArray.push_back(j);
                }
            }
        }
    }

    for (size_t i = 0; i < pr.size();i++ ){
        pr[i] = precision[insertArray[i]];
        ss[i] = score[insertArray[i]];
    }

    for (size_t i = 0; i < pr.size(); i++)
    {
        apSum = apSum + pr[i];
        scoreSum = scoreSum + ss[i];
    }

    double apMean,scoreMean;
    apMean = apSum / pr.size();
    scoreMean = scoreSum / ss.size();
    cout << "scoreMean:" << scoreMean << endl;
    std::vector<double> result;
    result.push_back(apMean);
    result.push_back(scoreMean);
    return result;
}


// 计算混淆矩阵
void EvaluationIndex::confusionMatrix(
    bool bboxTag,
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
                if (bboxTag){
                    BB db,gb;
                    double iou = 0;
                    // 构建和预测框真实框一样大小的数组
                    double dt[perImgPre.second[i].preBbox.size()];
                    double gt[gtInfo[perImgPre.first][j].gtBbox.size()];
                    // 把vector的真实框和预测框转化为数组
                    memcpy(dt,&perImgPre.second[i].preBbox[0],perImgPre.second[i].preBbox.size()*sizeof(perImgPre.second[i].preBbox[0]));
                    memcpy(gt,&gtInfo[perImgPre.first][j].gtBbox[0],gtInfo[perImgPre.first][j].gtBbox.size()*sizeof(gtInfo[perImgPre.first][j].gtBbox[0]));
                    // memcpy(dt,&preInfo[i].preBbox[0],preInfo[i].preBbox.size() * sizeof(preInfo[i].preBbox[0]));
                    // memcpy(gt,&gtInfo[j].gtBbox[k][0],gtInfo[j].gtBbox[k].size() * sizeof(gtInfo[j].gtBbox[k][0]));
                    db = dt;
                    gb = gt;
                    // 预测框个数
                    siz m = 1;
                    // 真实框个数
                    siz n = 1;
                    byte2 iscrowd = 0;
                    maskiou->bbIou(db,gb,m,n,&iscrowd,&iou);
                    ious[i][j] = iou;
                    // if(perImgPre.first == "000449"){
                    //     std::cout << "iou: " << ious[i][j] << std::endl;
                    // }
                }else{
                    ious[i][j] = rotateIOUcv(perImgPre.second[i].preRect,gtInfo[perImgPre.first][j].gtRect);
                    // cout << "perImgPre:" << perImgPre.first << endl;
                    // if (perImgPre.first == "000449"){
                    //     cout << "conMuIOU:" << ious[i][j] << endl;
                    // }
                }
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