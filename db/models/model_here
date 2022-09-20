#include "modelTrain.h"

ModelTrain::ModelTrain(QTextBrowser* Widget, QProgressBar* trainProgressBar):
    OutShow(Widget),
    trainProgressBar(trainProgressBar),
    process_train(new QProcess(this)){
    QObject::connect(process_train, &QProcess::readyReadStandardOutput, this, &ModelTrain::readLogOutput);
    QObject::connect(process_train, &QProcess::readyReadStandardError, this, &ModelTrain::readLogError);
}

ModelTrain::~ModelTrain(){

}

void ModelTrain::readLogOutput(){
    /* 读取终端输出并显示 */
    QByteArray cmdOut = process_train->readAllStandardOutput();
    if(!cmdOut.isEmpty()){
        QString logs=QString::fromLocal8Bit(cmdOut);
        QStringList lines = logs.split("\n");
        int len=lines.length();
        for(int i=0;i<len;i++){
            QStringList Infos = lines[i].simplified().split(" ");
            if(lines[i].contains("Train Ended",Qt::CaseSensitive)){
                OutShow->append("===========================Train Ended===========================");
                showLog=false;
                showTrianResult();
            }
            else if(showLog){
                OutShow->append(lines[i]);
            }
        }
    }
    OutShow->update();
}

void ModelTrain::readLogError(){
    /* 读取终端Error并显示 */
    QByteArray cmdOut = process_train->readAllStandardError();
    if(!cmdOut.isEmpty()){
        QString logs=QString::fromLocal8Bit(cmdOut);
        QStringList lines = logs.split("\n");
        int len=lines.length();
        for(int i=0;i<len-1;i++){
            OutShow->append(lines[i]);
        }
    }
    OutShow->update();
//    stopTrain();
}

void ModelTrain::startTrain(int modeltypeId,QString cmd){
  // TODO add code here
    modelTypeId = modeltypeId;
    if(process_train->state()==QProcess::Running){
        showLog=false;
        process_train->close();
        process_train->kill();
    }
    process_train->start("bash");
    showLog=true;
    OutShow->setText("===========================Train Starting===========================");
    trainProgressBar->setMaximum(0);
    trainProgressBar->setValue(0);
    process_train->setProcessChannelMode(QProcess::MergedChannels);
    process_train->write(cmd.toLocal8Bit() + '\n');
}

void ModelTrain::showTrianResult(){
    showLog=false;
    trainProgressBar->setMaximum(100);
    trainProgressBar->setValue(100);
}

void ModelTrain::stopTrain(){
    showLog=false;
    trainProgressBar->setMaximum(100);
    trainProgressBar->setValue(0);
    OutShow->append("===========================Train Stoping===========================");
    if(process_train->state()==QProcess::Running){
        process_train->close();
        process_train->kill();
    }
}
