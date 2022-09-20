#ifndef SEARCHFOLDER_H
#define SEARCHFOLDER_H

#include <iostream>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>
#include "./lib/guiLogic/tinyXml/tinyxml.h"
class SearchFolder{
    public:
        SearchFolder(){};
        ~SearchFolder(){};

        // 获取指定目录下的文件或文件夹名称
        bool getFiles(std::vector<std::string> &files, std::string filesType, std::string folderPath);
        bool getDirs(std::vector<std::string> &dirs, std::string folderPath);
        
        // 从TXT中读取ground truth
        bool getGroundTruth(std::vector<std::string>  &label_GT, std::vector<std::vector<cv::Point>> &points_GT, std::string labelPath);


        // 从XML中读取ground truth
        bool getGtXML(std::vector<std::string>  &label_GT, std::vector<std::vector<cv::Point>> &points_GT, std::string labelPath);
    private:

};


#endif // SEARCHFOLDER_H
