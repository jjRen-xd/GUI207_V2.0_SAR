#include "dataEvalPage.h"

#include <QMessageBox>
#include <QFileDialog>
#include <algorithm>
#include <cstdio>

using namespace std;
#define PI 3.1415926

DataEvalPage::DataEvalPage(Ui_MainWindow *main_ui,
                           BashTerminal *bash_terminal,
                           DatasetInfo *globalDatasetInfo,
                           ModelInfo *globalModelInfo,
                           TorchServe *globalTorchServe,
                           EvaluationIndex *evalBack) : ui(main_ui),
                                                        terminal(bash_terminal),
                                                        datasetInfo(globalDatasetInfo),
                                                        modelInfo(globalModelInfo),
                                                        torchServe(globalTorchServe),
                                                        eval(evalBack)
{
    refreshGlobalInfo();
    connect(ui->pushButton_mE_testAll, &QPushButton::clicked, this, &DataEvalPage::ttThread);

    uiResult.emplace("mAP50", ui->label_map50Num);
    uiResult.emplace("mPrec", ui->label_precNum);
    uiResult.emplace("mRecall", ui->label_recNum);
    uiResult.emplace("mcfar", ui->label_cfarNum);
    uiResult.emplace("gtAll", ui->label_gtNum);
    uiResult.emplace("preAll", ui->label_dtNum);
    uiResult.emplace("tp", ui->label_tpNum);
    uiResult.emplace("score", ui->label_scoreNum);

    resultMean.emplace("mAP50", 0.0);
    resultMean.emplace("mPrec", 0.0);
    resultMean.emplace("mRecall", 0.0);
    resultMean.emplace("mcfar", 0.0);
    resultMean.emplace("gtAll", 0.0);
    resultMean.emplace("preAll", 0.0);
    resultMean.emplace("tp", 0.0);
    resultMean.emplace("score", 0.0);

    thread = new TestThread(datasetInfo, eval, &(this->choicedDatasetPATH), &(this->choicedModelName), &(this->choicedModelType));
    connect(thread, &TestThread::end, this, &DataEvalPage::outThread);
}

DataEvalPage::~DataEvalPage()
{
}

void DataEvalPage::ttThread()
{
    ui->pushButton_mE_testAll->setEnabled(false);
    if (nullptr != thread)
    {
        thread->start();
    }
}

//线程结束触发槽函数，用于打印线程输出的结果
void DataEvalPage::outThread(std::vector<result_> result, std::vector<std::string> classType, std::map<std::string, std::map<std::string, float>> conMatrix)
{
    resultMean.clear();
    std::vector<std::string> matrixType(classType);
    matrixType.push_back("background");
    if (result.size() == 0)
    {
        QMessageBox::warning(NULL, "错误", "结果为空！");
    }
    else
    {
        for (size_t i = 0; i < classType.size(); i++)
        {
            resultMean["mAP50"] = resultMean["mAP50"] + result[i].ap;
            resultMean["mPrec"] = resultMean["mPrec"] + result[i].precision;
            resultMean["mRecall"] = resultMean["mRecall"] + result[i].recall;
            resultMean["mcfar"] = resultMean["mcfar"] + result[i].cfar;
            resultMean["gtAll"] = resultMean["gtAll"] + float(result[i].gtNUm);
            resultMean["preAll"] = resultMean["preAll"] + float(result[i].detNUm);
            resultMean["tp"] = resultMean["tp"] + float(result[i].tp);
            resultMean["score"] = resultMean["score"] + float(result[i].score);
        }
        resultMean["mAP50"] = resultMean["mAP50"] / classType.size();
        resultMean["mPrec"] = resultMean["mPrec"] / classType.size();
        resultMean["mRecall"] = resultMean["mRecall"] / classType.size();
        resultMean["mcfar"] = resultMean["mcfar"] / classType.size();
        resultMean["score"] = resultMean["score"] / classType.size();
        updateUiResult();
        plotConMatrix(conMatrix, matrixType);
        histogram(result, resultMean, classType);
        ui->pushButton_mE_testAll->setEnabled(true);
    }
}

void DataEvalPage::updateUiResult()
{
    for (auto subResult : this->uiResult)
    {
        std::string type = subResult.first;
        if (type == "gtAll" || type == "preAll" || type == "tp")
        {
            subResult.second->setText(QString("%1").arg(resultMean[subResult.first]));
        }
        else
        {
            subResult.second->setText(QString::number(resultMean[subResult.first] * 100, 'f', 2).append("%"));
        }
    }
}

void DataEvalPage::refreshGlobalInfo()
{
    // 基本信息更新
    if(modelInfo->checkMap(modelInfo->selectedType, modelInfo->selectedName, "batch")){
        this->choicedModelName = QString::fromStdString(modelInfo->selectedName);
        this->choicedModelType = QString::fromStdString(modelInfo->selectedType);
        ui->label_mE_batch_2->setText(QString::fromStdString(modelInfo->getAttri(modelInfo->selectedType, modelInfo->selectedName, "batch")));
        ui->label_mE_model_2->setText(choicedModelName);
    }
    else{
        this->choicedModelName = "";
        this->choicedModelType = "";
        ui->label_mE_batch_2->setText("");
        ui->label_mE_model_2->setText("空");
    }
    if(datasetInfo->checkMap(datasetInfo->selectedType, datasetInfo->selectedName, "PATH")){
        ui->label_mE_dataset_2->setText(QString::fromStdString(datasetInfo->selectedName));
        this->choicedDatasetPATH = datasetInfo->getAttri(datasetInfo->selectedType,datasetInfo->selectedName,"PATH");
    }
    else{
        this->choicedDatasetPATH = "";
        ui->label_mE_dataset_2->setText("空");
    }

}

float DataEvalPage::rotateIOUcv(cv::RotatedRect rect1, cv::RotatedRect rect2)
{
    float areaRect1 = rect1.size.width * rect1.size.height;
    float areaRect2 = rect2.size.width * rect2.size.height;
    vector<cv::Point2f> vertices;
    int intersectionType = cv::rotatedRectangleIntersection(rect1, rect2, vertices);
    if (vertices.size() == 0)
        return 0.0;
    else
    {
        vector<cv::Point2f> order_pts;
        // 找到交集（交集的区域），对轮廓的各个点进行排序
        cv::convexHull(cv::Mat(vertices), order_pts, true);
        double area = cv::contourArea(order_pts);
        float inner = (float)(area / (areaRect1 + areaRect2 - area + 0.0001));
        return inner;
    }
}

// 绘制混淆矩阵
void DataEvalPage::plotConMatrix(std::map<std::string, std::map<std::string,
                                                                float>>
                                     conMatrix,
                                 std::vector<std::string> matrixType)
{
    ui->qTable->setWindowTitle("Confusion Matrix");
    ui->qTable->setColumnCount(matrixType.size());
    ui->qTable->setRowCount(matrixType.size());
    // 行列标题设置
    QStringList h_Header;
    QStringList v_Header;
    for (size_t i = 0; i < matrixType.size(); i++)
    {
        h_Header.append(QString::fromStdString(matrixType[i]));
        v_Header.append(QString::fromStdString(matrixType[i]));
    }
    ui->qTable->setHorizontalHeaderLabels(h_Header);
    ui->qTable->setVerticalHeaderLabels(v_Header);
    ui->qTable->setStyleSheet("QHeaderView::section{background:white;}");
    ui->qTable->horizontalHeader()->setVisible(true);
    ui->qTable->verticalHeader()->setVisible(true);
    //窗口自适应
    // ui->qTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // ui->qTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // 不可以编辑
    ui->qTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 根据内容设置行列长度
    // ui->qTable->resizeColumnsToContents();
    // ui->qTable->resizeRowsToContents();

    std::vector<float> iSum;
    for (size_t i = 0; i < matrixType.size(); i++)
    {
        float iSumTmp = 0;
        for (size_t j = 0; j < matrixType.size(); j++)
        {
            iSumTmp = iSumTmp + conMatrix[matrixType[i]][matrixType[j]];
        }
        iSum.push_back(iSumTmp);
    }
    // 单元格赋值
    for (size_t i = 0; i < matrixType.size(); i++)
    {
        for (size_t j = 0; j < matrixType.size(); j++)
        {

            if (iSum[i] > 0)
            {
                float res = conMatrix[matrixType[i]][matrixType[j]] / iSum[i] * 100;
                ui->qTable->setItem(i, j, new QTableWidgetItem(QString::number(res, 'f', 0).append("%")));
                ui->qTable->item(i, j)->setTextAlignment(Qt::AlignCenter);
            }
            else
            {
                ui->qTable->setItem(i, j, new QTableWidgetItem(QString::number(0).append("%")));
            }
        }
    }
    // 单元格细节美化
    for (size_t i = 0; i < matrixType.size(); i++)
    {
        for (size_t j = 0; j < matrixType.size(); j++)
        {
            // 文本居中
            ui->qTable->item(i, j)->setTextAlignment(Qt::AlignCenter);
        }
    }
}

void removeLayout(QLayout *layout)
{
    QLayoutItem *child;
    if (layout == nullptr)
        return;
    while ((child = layout->takeAt(0)) != nullptr)
    {
        // child可能为QLayoutWidget、QLayout、QSpaceItem
        // QLayout、QSpaceItem直接删除、QLayoutWidget需删除内部widget和自身
        if (QWidget *widget = child->widget())
        {
            widget->setParent(nullptr);
            delete widget;
            widget = nullptr;
        }
        else if (QLayout *childLayout = child->layout())
            removeLayout(childLayout);
        delete child;
        child = nullptr;
    }
}

// 绘制柱状图
void DataEvalPage::histogram(std::vector<result_> result, std::map<std::string, float> resultMean, std::vector<std::string> classType)
{
    //创建条形组
    std::vector<float> ap50;
    std::vector<float> recall;
    std::vector<float> score;
    std::vector<std::string> chartType(classType);
    chartType.push_back("平均");
    for (size_t i = 0; i < classType.size(); i++)
    {
        ap50.push_back(result[i].ap);
        recall.push_back(result[i].recall);
        score.push_back(result[i].score);
    }
    // 加入总的值
    ap50.push_back(resultMean["mAP50"]);
    recall.push_back(resultMean["mRecall"]);
    score.push_back(resultMean["score"]);

    QChart *chart = new QChart;
    std::map<QString, vector<float>> mapnum;
    mapnum.insert(pair<QString, std::vector<float>>("AP50", ap50)); //后续可拓展
    mapnum.insert(pair<QString, std::vector<float>>("Recall", recall));
    mapnum.insert(pair<QString, std::vector<float>>("Score", score));

    QBarSeries *series = new QBarSeries();
    map<QString, vector<float>>::iterator it = mapnum.begin();
    //将数据读入
    while (it != mapnum.end())
    {
        QString tit = it->first;
        QBarSet *set = new QBarSet(tit);
        std::vector<float> vecnum = it->second;
        for (auto a : vecnum)
        {
            // 百分号
            a = a * 100;
            // 保留两位
            a = ((float)((int)((a + 0.005) * 100))) / 100;
            *set << a;
        }
        series->append(set);
        it++;
    }
    // QString::number(resultMean[subResult.first],'f',2).append("%")
    series->setVisible(true);
    series->setLabelsVisible(true);
    // 横坐标参数
    QBarCategoryAxis *axis = new QBarCategoryAxis;
    for (int i = 0; i < chartType.size(); i++)
    {
        axis->append(QString::fromStdString(chartType[i]));
    }
    QValueAxis *axisy = new QValueAxis;
    axisy->setRange(0, 100);
    axisy->setTitleText("%");
    QFont chartLabel;
    chartLabel.setPixelSize(14);
    chart->addSeries(series);
    chart->setTitle("各类别具体参数图");
    chart->setTitleFont(chartLabel);

    chart->setAxisX(axis, series);
    chart->setAxisY(axisy, series);
    chart->legend()->setVisible(true);

    QChartView *view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    removeLayout(ui->horizontalLayout_histogram);
    ui->horizontalLayout_histogram->addWidget(view);
}
