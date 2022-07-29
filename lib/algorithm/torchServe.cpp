#include "torchServe.h"

#include "./lib/guiLogic/tools/delayTools.h"

#include <QString>
#include <QThread>

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
        {"RBOX_DET", {{"Inference", 8080}, {"Management", 8081}, {"Metrics", 8082}}}
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
-p8080:8080 -p8081:8081 -p8082:8082 \
--mount type=bind,\
source=/media/z840/HDD_1/LINUX/GUI207_V2.0_SAR/db/models,\
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
QString TorchServe::inferenceOne(QString modelName, QString dataPath){

}


// TODO
QString TorchServe::inferenceAll(QString modelName, QString datasetPath){


}   


// 推理结果解析接口
std::vector<std::map<QString,QString>> TorchServe::parseInferenceResult(QString resultStr){

}
