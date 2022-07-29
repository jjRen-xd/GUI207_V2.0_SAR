#ifndef DELAYTOOLS_H
#define DELAYTOOLS_H

#include <iostream>

#include <QTime>
#include <QTimer>
#include <QEventLoop>
#include <QCoreApplication>

namespace DELAY 
{

// 1、阻塞型延时
// 阻塞的原理：在延时期间，本线程的事件循环得不到执行。

// <1>：
// 多线程程序使用QThread::sleep()或者QThread::msleep()或QThread::usleep()或QThread::wait()进行延时处理。这几个函数带来的不良效果就是：GUI会在延时的时间段内失去响应，界面卡死。所以，这三个函数一般用在非GUI线程中。

// <2>：
void Delay_MSec_Suspend(unsigned int msec)
{    
    QTime _Timer = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < _Timer );
}


// 2、非阻塞型延时
// 原理就是利用事件循环，也有两种方式：

// <1>、处理本线程的事件循环
///
/// \brief sleep_msec
/// \param msec
/// \note 这种方法不会阻塞当前线程,尤其适合Qt单线程带UI程序
void sleep_msec(int msec)
{
    std::cout<<"start time";
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < dieTime )
    {
        //强制进入当前线程的事件循环,这样可以把堵塞的事件都处理掉,从而避免程序卡死。
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        //如果去掉QCoreApplication::processEvents; 可以延时,但会阻塞线程
    }
    std::cout<<"out of time";
}

// <2>、使用子事件循环
///
/// \brief delay_msec
/// \param msec
/// \note 创建子事件循环,在子事件循环中,父事件循环仍然是可以执行的,该方法不会阻塞线程
void delay_msec(unsigned int msec)
{
    //定义一个新的事件循环
    QEventLoop loop;
    //创建单次定时器,槽函数为事件循环的退出函数
    QTimer::singleShot(msec, &loop, SLOT(quit()));
    //事件循环开始执行,程序会卡在这里,直到定时时间到,本循环被退出
    loop.exec();
}

}
#endif // DELAYTOOLS_H
