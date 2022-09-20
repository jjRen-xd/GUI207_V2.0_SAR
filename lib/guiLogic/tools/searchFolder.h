#ifndef SEARCHFOLDER_H
#define SEARCHFOLDER_H

#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <opencv2/opencv.hpp>

class SearchFolder{
    public:
        SearchFolder(){};
        ~SearchFolder(){};

        // 获取指定目录下的文件或文件夹名称
        bool getFiles(std::vector<std::string> &files, std::string filesType, std::string folderPath);
        bool getDirs(std::vector<std::string> &dirs, std::string folderPath);

        // 判断文件是否存在
        bool exist(const std::string& name);
        
        // 从TXT中读取ground truth
        bool getGroundTruth(std::vector<std::string>  &label_GT, std::vector<std::vector<cv::Point>> &points_GT, std::string labelPath);
    private:

};


#endif // SEARCHFOLDER_H
