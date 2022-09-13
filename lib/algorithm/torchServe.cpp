#include "torchServe.h"

#include "./lib/guiLogic/tools/delayTools.h"

#include <QString>
#include <QThread>

#include <iostream>
#include <string>
#include <regex>
#include <vector>

TorchServe::TorchServe(BashTerminal *bash_terminal, ModelInfo *globalModelInfo):
    terminal(bash_terminal),
    modelInfo(globalModelInfo)
{
    // 初始化TorchServe服务器端口
    serverPortList = {
        {"TRA_DL",   {{"Inference", 0}, {"Management", 0}, {"Metrics", 0}}},
        {"FEA_RELE", {{"Inference", 0}, {"Management", 0}, {"Metrics", 0}}},
        {"FEW_SHOT", {{"Inference", 0}, {"Management", 0}, {"Metrics", 0}}},
        {"FEA_OPTI", {{"Inference", 0}, {"Management", 0}, {"Metrics", 0}}},
        {"RBOX_DET", {{"Inference", 7080}, {"Management", 7081}, {"Metrics", 7082}}}
    };
    initTorchServe();
}


TorchServe::~TorchServe(){

}


// 管理接口
int TorchServe::initTorchServe(){
    this->terminal->print("初始化TorchServe");
    // 初始化Docker
    this->terminal->execute("docker stop $(docker ps -aq) && docker rm $(docker ps -aq)");
    QString dockerRunCmd = "gnome-terminal -x bash -c \"\
docker run --rm \
--cpus 16 \
--gpus all \
-p7080:8080 -p7081:8081 -p7082:8082 \
--mount type=bind,\
source=/run/media/dell_3090/软件/jwk/207/GUI207_V2.0_SAR/db/models,\
target=/home/model-server/model-store mmrotate-serve:latest; \
exec bash -l\"";
    this->terminal->execute(dockerRunCmd);
    // 延时
    DELAY::sleep_msec(15000);
    // QThread::sleep(20);

    // 根据已经导入的模型，上传.mar模型至torchServe服务器
    std::vector<std::string> modelTypes = modelInfo->getTypes();
    for(auto &modelType: modelTypes){
        std::vector<std::string> modelNames = modelInfo->getNamesInType(modelType);
        for(auto &modelName:modelNames){
            std::string modelPath = modelInfo->getAttri(modelType, modelName, "PATH");
            // 异常处理
            if(modelPath.empty() || modelPath == "未定义"){
                terminal->print("模型" + QString::fromStdString(modelName) + "未定义路径, 跳过上传");
                continue;
            }
            if(modelPath.find(".mar") == std::string::npos){
                terminal->print("模型" + QString::fromStdString(modelName) + "不是.mar格式, 跳过上传");
                continue;
            }
            // 上传模型
            this->postModel(QString::fromStdString(modelName), QString::fromStdString(modelType), 2);
        }
    }
    return 1;
}


int TorchServe::postModel(QString modelName, QString modelType, int numWorkers){
    QString torchServePOST = "curl -X POST \"http://localhost:"+
                                QString::number(serverPortList[modelType]["Management"])+
                                "/models?initial_workers="+
                                QString::number(numWorkers)+"&url="+modelName+'\"';
    this->terminal->execute(torchServePOST);
    return 1;
}


int TorchServe::deleteModel(QString modelName, QString modelType){
    QString torchServeDELETE = "curl -X DELETE \"http://localhost:"+
                                QString::number(serverPortList[modelType]["Management"])+
                                "/models/"+modelName.split(".")[0]+'\"';
    this->terminal->execute(torchServeDELETE);
    return 1;
}


QString TorchServe::getModelList(){

}


// 推理接口
std::vector<std::map<QString,QString>> TorchServe::inferenceOne(QString modelName, QString modelType, QString dataPath){
    QString torchServeInfer = "curl http://127.0.0.1:"+
                                QString::number(serverPortList[modelType]["Inference"])+
                                "/predictions/" + modelName + " -T" + dataPath;
    QString respones;
    auto parsedMap = std::vector<std::map<QString,QString>>();

    this->terminal->execute(torchServeInfer, &respones);
    // std::cout<<respones.toStdString()<<std::endl;
    parseInferenceResult(respones, parsedMap);

    return parsedMap;
}


// TODO
QString TorchServe::inferenceAll(QString modelName, QString datasetPath){


}   


// 推理结果解析接口
void TorchServe::parseInferenceResult(QString resultStr, std::vector<std::map<QString,QString>> &parsedMap){
    resultStr = resultStr.simplified();
    resultStr = resultStr.remove(' ');
    std::vector<QString> samplesStr = getRegex(resultStr.toStdString(), std::string("\\{(.+?)\\}"));
    for(auto &sampleStr: samplesStr){   // 对于每个识别个体，包含class_name, bbox, score共三个key
        std::map<QString,QString> sampleMap;
        QStringList attriStr = sampleStr.split(",\"");  // 分离三个key
        for(auto &attri: attriStr){
            attri = attri.remove(QChar('\"'));
            attri = attri.remove(QChar(' '));
            attri = attri.remove(QChar('{'));
            attri = attri.remove(QChar('}'));
            QStringList attriKeyValue = attri.split(":");
            if(attriKeyValue.size() == 2){
                sampleMap[attriKeyValue[0]] = attriKeyValue[1];
            }
        }
        parsedMap.push_back(sampleMap);
    }
        // inferenceResult.push_back(sampleMap);
}

// 正则表达式匹配接口
std::vector<QString> TorchServe::getRegex(std::string s, std::string pattern){
    auto res = std::vector<QString>();
    std::regex r(pattern);
    std::sregex_iterator pos(s.cbegin(), s.cend(), r), end;
    for (; pos != end; ++pos)
        res.push_back(QString::fromStdString(pos->str(0)));
    return res;
}

/* 
[
  {
    "class_name": "ship",
    "bbox": [
      331.0644226074219,
      274.12567138671875,
      44.357879638671875,
      16.865642547607422,
      1.1950498819351196
    ],
    "score": 0.9955195188522339
  },
  {
    "class_name": "ship",
    "bbox": [
      156.04086303710938,
      65.87517547607422,
      41.56586837768555,
      14.430924415588379,
      1.1693059206008911
    ],
    "score": 0.9905607104301453
  }
]
*/