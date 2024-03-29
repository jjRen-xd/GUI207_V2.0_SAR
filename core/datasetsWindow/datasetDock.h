#ifndef DATASETDOCK_H
#define DATASETDOCK_H

#include <QObject>
#include "ui_MainWindow.h"

#include "./lib/guiLogic/bashTerminal.h"
#include "./lib/guiLogic/datasetInfo.h"
// #include "core/datasetsWindow/chart.h"

#include "./lib/guiLogic/tools/searchFolder.h"
class DatasetDock:public QObject{
    Q_OBJECT
public:
    DatasetDock(Ui_MainWindow *main_ui, BashTerminal *bash_terminal, DatasetInfo *globalDatasetInfo);
    ~DatasetDock();

    // 组信息
    std::map<std::string, QLabel*> attriLabelGroup;
    std::map<std::string, QTreeView*> datasetTreeViewGroup;
    std::vector<QLabel*> imageViewGroup;
    std::vector<QLabel*> imageInfoGroup;

    void reloadTreeView();

    std::string previewType;
    std::string previewName;
public slots:
    void importDataset(std::string type);
    void deleteDataset();

private slots:
    void treeItemClicked(const QModelIndex &index);
private:
    Ui_MainWindow *ui;
    BashTerminal *terminal;

    DatasetInfo *datasetInfo;


    // 不同平台下文件夹搜索工具
    SearchFolder *dirTools = new SearchFolder();
};

#endif // DATASETDOCK_H
