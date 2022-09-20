#ifndef MYSTRUCT_H
#define MYSTRUCT_H

    struct gt_info
    {
        std::string imgName;
        std::vector<cv::RotatedRect> gtRect;        // 一个图片有多个gt
        std::vector<int> det;       // 每个gt所对应的匹配标志
    };
    struct pre_info
    {
        std::string imgName;
        cv::RotatedRect preRect;
        float score;        // 置信度
        bool operator <(const pre_info &x)const
        {
            return score>x.score; //降序排列
        }
    };
    //混淆矩阵存储格式
    struct gt_info_cm
    {
        cv::RotatedRect gtRect;        // 一个图片有多个gt
        std::string className;
    };

    struct pre_info_cm
    {
        cv::RotatedRect preRect;
        float score;        // 置信度
        std::string className;
    };

    struct result_
    {
        std::string className;
        int gtNUm;
        int detNUm;
        float fp;
        float tp;
        float ap;
        float recall;
        float precision;
        float cfar;
        float score;
    };


#endif