#ifndef MYSTRUCT_H
#define MYSTRUCT_H

    struct gt_info
    {
        std::string imgName;
        // 二维数组，因为一个图片有多个gt
        std::vector<std::vector<std::double_t>> gtBbox;
        std::vector<cv::RotatedRect> gtRect;        // 一个图片有多个gt
        std::vector<int> det;       // 每个gt所对应的匹配标志
    };
    struct pre_info
    {
        std::string imgName;
        // 正框用到的存储格式，左下角x,y,w,h
        std::vector<std::double_t> preBbox;
        // 存储斜框的数据为rect
        cv::RotatedRect preRect;
        double score;        // 置信度
        bool operator <(const pre_info &x)const
        {
            return score>x.score; //降序排列
        }
    };
    //混淆矩阵存储格式
    struct gt_info_cm
    {
        std::vector<std::double_t> gtBbox;
        cv::RotatedRect gtRect;        // 一个图片有多个gt
        std::string className;
    };

    struct pre_info_cm
    {
        std::vector<std::double_t> preBbox;
        cv::RotatedRect preRect;
        double score;        // 置信度
        std::string className;
    };

    struct result_
    {
        std::string className;
        int gtNUm;
        int detNUm;
        double fp;
        double tp;
        double ap;
        double recall;
        double precision;
        double cfar;
        double score;
    };


#endif