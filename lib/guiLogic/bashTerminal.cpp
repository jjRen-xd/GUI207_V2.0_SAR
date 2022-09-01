#include "bashTerminal.h"

BashTerminal::BashTerminal(QLineEdit *inWidget, QTextEdit *outWidget):
    bashInEdit(inWidget),
    bashOutShow(outWidget),
    process_bash(new QProcess(this)){  

    process_bash->start(bashApi);
    inWidget->setPlaceholderText("Your command");
    bashOutShow->setLineWrapMode(QTextEdit::NoWrap);
    process_bash->setProcessChannelMode(QProcess::MergedChannels);
    QObject::connect(process_bash, &QProcess::readyReadStandardOutput, this, &BashTerminal::readBashOutput);
    QObject::connect(process_bash, &QProcess::readyReadStandardError, this, &BashTerminal::readBashError);

}


BashTerminal::~BashTerminal(){
    process_bash->terminate();
}


void BashTerminal::print(QString msg){
    bashOutShow->append(msg);
    bashOutShow->update();
}


void BashTerminal::execute(QString cmd){
    /* 开放在终端运行命令接口 */
    // this->print("$执行命令: \n" + cmd + "\n");
    bashOutShow->append("Shell:~$ "+cmd);
    process_bash->write(cmd.toLocal8Bit() + '\n');
}

void BashTerminal::execute(QString cmd, QString* output){
    /* 阻塞式运行命令接口, 并保存结果 */
    QProcess p;
    p.start(bashApi, QStringList() <<"-c" << cmd);
    p.waitForFinished();
    QString strResult = p.readAllStandardOutput();
    // this->print(strResult);
    *output = strResult;
}


void BashTerminal::commitBash(){
    /* 在GUI上手动输入向终端提交命令 */
    QString cmdIn = bashInEdit->text();
    bashOutShow->append("Shell:~$ "+cmdIn);
    process_bash->write(cmdIn.toLocal8Bit() + '\n');
    bashInEdit->clear();
}


void BashTerminal::cleanBash(){
    /* 清空并重启终端 */
    bashOutShow->clear();
    process_bash->close();
    process_bash->start("powershell");
}


void BashTerminal::readBashOutput(){
    /* 读取终端输出并显示 */
    QByteArray cmdOut = process_bash->readAllStandardOutput();
    if(!cmdOut.isEmpty()){
        bashOutShow->append(QString::fromLocal8Bit(cmdOut));
    }
    bashOutShow->update();
    QString* message = new QString(QString::fromLocal8Bit(cmdOut));
    this->mesQueue.push(message);
}


void BashTerminal::readBashError(){
    /* 读取终端Error并显示 */
    QByteArray cmdOut = process_bash->readAllStandardError();
    if(!cmdOut.isEmpty()){
        bashOutShow->append(QString::fromLocal8Bit(cmdOut));
    }
    bashOutShow->update();

    QString* error = new QString(QString::fromLocal8Bit(cmdOut));
    this->errQueue.push(error);
}
