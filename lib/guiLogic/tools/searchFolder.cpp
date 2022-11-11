#include "searchFolder.h"
#include "qdebug.h"
#include <string.h>
#include <string>
#include <fstream>
#include <QObject>

using namespace std;

// 为了兼容win与linux双平台
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <io.h>
// Windows平台
bool SearchFolder::getFiles(vector<string> &files, string filesType, string folderPath){
    intptr_t hFile = 0;
    struct _finddata_t fileInfo;

    if ((hFile = _findfirst((folderPath+"/*"+filesType).c_str(), &fileInfo)) != -1){
        do{
            files.push_back(fileInfo.name);
        } while(_findnext(hFile, &fileInfo) == 0);
    }
    else{
        return false;
    }
    return true;
}

// bool SearchFolder::getDirs(vector<string> &dirs, string folderPath){
//     intptr_t hFile = 0;
//     struct _finddata_t fileInfo;

//     if ((hFile = _findfirst((folderPath+"/*").c_str(), &fileInfo)) != -1){
//         do{
//             if ((fileInfo.attrib & _A_SUBDIR) && strcmp(fileInfo.name, ".") != 0 && strcmp(fileInfo.name, "..") != 0) {  //比较文件类型是否是文件夹
//                 dirs.push_back(fileInfo.name);
//             }
//         } while(_findnext(hFile, &fileInfo) == 0);
//     }
//     else{
//         return false;
//     }
//     return true;
// }

#else
#include <dirent.h>
// Linux平台
bool SearchFolder::getFiles(vector<string> &files, string filesType, string folderPath){
    DIR *dir;
    struct dirent *ptr;

    if ((dir=opendir(folderPath.c_str())) == NULL)
        return false;

    while ((ptr=readdir(dir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)    ///current dir OR parrent dir
            continue;
        else if (ptr->d_type == 8){    //file
            // 判断是否是指定类型

            string sFilename(ptr->d_name);
            string suffixStr = sFilename.substr(sFilename.find_last_of('.'));//获取文件后缀
            if (suffixStr.compare(filesType) == 0) {//根据后缀筛选文件
                files.push_back(ptr->d_name);
            }
        }
        else if (ptr->d_type == 10)    //link file
            continue;
        else if (ptr->d_type == 4)    //dir
            continue;
    }
    closedir(dir);
    return true;
}

bool SearchFolder::getDirs(vector<string> &dirs, string folderPath){
    struct dirent *ptr;
    DIR *dir;

    if ((dir=opendir(folderPath.c_str())) == NULL)
        return false;

    while ((ptr=readdir(dir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)    ///current dir OR parrent dir
            continue;
        else if (ptr->d_type == 8)    ///file
            continue;
        else if (ptr->d_type == 10)    ///link file
            continue;
        else if (ptr->d_type == 4)    ///dir
            dirs.push_back(ptr->d_name);
    }
    closedir(dir);
    return true;
}

#endif


bool SearchFolder::getGroundTruth(
    vector<string>  &label_GT,
    vector<vector<cv::Point>> &points_GT,
    string labelPath)
{
    ifstream infile(labelPath);
    assert(infile.is_open());   // 文件打开失败
    string line;
    while(getline(infile,line)){
        QStringList locInfo = QString::fromStdString(line).split(' ');
        vector<cv::Point> currPoints = {
            cv::Point(locInfo[0].toFloat(),locInfo[1].toFloat()),
            cv::Point(locInfo[2].toFloat(),locInfo[3].toFloat()),
            cv::Point(locInfo[4].toFloat(),locInfo[5].toFloat()),
            cv::Point(locInfo[6].toFloat(),locInfo[7].toFloat())
        };
        label_GT.push_back(locInfo[8].toStdString());
        points_GT.push_back(currPoints);
    }
    infile.close();
}


bool SearchFolder::getGtXML(
    std::vector<std::string>  &label_GT,
    std::vector<std::vector<cv::Point>> &points_GT,
    std::vector<std::vector<std::double_t>> &bboxGT,
    std::string labelPath)
{
    TiXmlDocument doc;
    if(!doc.LoadFile(labelPath.c_str()))
    {
        cerr << doc.ErrorDesc() << endl;
        return -1;
    }
    TiXmlElement* root = doc.FirstChildElement();
    if(root == NULL)
    {
        cerr << "Failed to load file: No root element." << endl;
        doc.Clear();
        return -1;
    }
    // cout<< "1: " <<root->Value()<<endl;//根节点 annotation
    for(TiXmlElement* elem = root->FirstChildElement(); elem != NULL; elem = elem->NextSiblingElement())
    {
        if(elem->FirstChildElement() && elem->ValueTStr() == "object")//嵌套有子节点
        {
            for (TiXmlElement* childelem=elem->FirstChildElement();childelem!=NULL;childelem=childelem->NextSiblingElement())
            {
                if(childelem->ValueTStr() == "name" )//子节点中还有子节点
                {
                    label_GT.push_back(childelem->GetText());
                }else if (childelem->ValueTStr() == "bndbox")
                {
                    std::vector<float> coordi;
                    for (TiXmlElement* local=childelem->FirstChildElement();local!=NULL;local=local->NextSiblingElement())
                    {
                        coordi.push_back(atof(local->GetText()));
                    }
                    // mmdet里，xmin和ymin要-1，会影响到最后的精度
                    float xmin = coordi[0]-1;
                    float ymin = coordi[1]-1;
                    float xmax = coordi[2];
                    float ymax = coordi[3];
                    float w = xmax - xmin;
                    float h = ymax - ymin;
                    std::vector<std::double_t> currBbox = {xmin,ymin,w,h};
                    vector<cv::Point> currPoints = {
                        cv::Point(xmin,ymax),
                        cv::Point(xmin,ymin),
                        cv::Point(xmax,ymin),
                        cv::Point(xmax,ymax)
                    };
                    points_GT.push_back(currPoints);
                    bboxGT.push_back(currBbox);
                }
            }
        }
    }
    doc.Clear();
}


bool SearchFolder::exist(const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}
