#ifndef TORCH_SERVE_H
#define TORCH_SERVE_H

#include <iostream>
#include <QString>
#include <string>
#include <vector>
#include <map>

// 命令行终端
#include "./lib/guiLogic/bashTerminal.h"
// 模型记录信息
#include "./lib/guiLogic/modelInfo.h"

class TorchServe{
    public:
        TorchServe(BashTerminal *bash_terminal, ModelInfo *globalModelInfo);
        ~TorchServe();

        // 管理接口
        int initTorchServe();
        int postModel(QString modelName, QString modelType, int numWorkers);
        int deleteModel(QString modelName, QString modelType);
        QString getModelList();

        // 推理接口
        QString inferenceOne(QString modelName, QString dataPath);
        QString inferenceAll(QString modelName, QString datasetPath);   // TODO

    private:
        // 推理结果解析接口
        std::vector<std::map<QString,QString>> parseInferenceResult(QString resultStr);

        // 各个TorchServe的管理接口
        std::map<QString, std::map<QString, int>> serverPortList;

        BashTerminal *terminal;
        ModelInfo *modelInfo;

};

#endif // TORCH_SERVE_H