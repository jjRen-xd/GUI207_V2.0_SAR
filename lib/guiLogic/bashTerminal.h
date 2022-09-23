#ifndef BASHTERMINAL_H
#define BASHTERMINAL_H

#include <QObject>
#include <QProcess>
#include <QLineEdit>
#include <QTextEdit>

#include <queue>


class BashTerminal: public QObject{
    Q_OBJECT

    public:
        BashTerminal(QLineEdit *inWidget, QTextEdit *outWidget);
        ~BashTerminal();

        std::queue<QString*> mesQueue;        // 维护一个消息队列，用于终端输出消息的排队
        std::queue<QString*> errQueue;        // 维护一个错误队列，用于终端输出错误的排队  

        // 为了兼容win与linux双平台
        #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
        QString bashApi = "powershell";            // "Windows" or "Linux"
        #else
        QString bashApi = "bash";            // "Windows" or "Linux"
        #endif

    public slots:
        void print(QString msg);     // 开放在终端打印str的接口
        void execute(QString cmd);   // 开放在终端运行命令接口
        void execute(QString cmd, QString* output);   // 阻塞式运行命令接口,并保存结果

        void commitBash();          // 手动输入向终端提交命令
        void cleanBash();           // 清空并重启终端

    private:
        QProcess *process_bash;
        QLineEdit *bashInEdit;      // 终端命令行输入
        QTextEdit *bashOutShow;     // 终端命令行输出


    private slots:
        void readBashOutput();      // 读取终端输出并显示
        void readBashError();       // 读取终端Error并显示
};

#endif // BASHTERMINAL_H
