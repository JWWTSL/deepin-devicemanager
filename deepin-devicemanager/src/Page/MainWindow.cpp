// 项目自身文件
#include "MainWindow.h"

// Qt库文件
#include <QResizeEvent>
#include <QDateTime>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QDir>
#include <QVBoxLayout>

// Dtk头文件
#include <DFileDialog>
#include <DApplication>
#include <DFontSizeManager>

// 其它头文件
#include "WaitingWidget.h"
#include "DeviceWidget.h"
#include "MacroDefinition.h"
#include "ThreadPool.h"
#include "deviceinfoparser.h"
#include "DeviceManager.h"
#include "commondefine.h"

DWIDGET_USE_NAMESPACE

// 主界面需要的一些宏定义
#define INIT_WIDTH  1000    // 窗口的初始化宽度
#define INIT_HEIGHT 720     // 窗口的初始化高度
#define MIN_WIDTH  840      // 窗口的最小宽度
#define MIN_HEIGHT 420      // 窗口的最小高度

MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent)
    , mp_MainStackWidget(new DStackedWidget(this))
    , mp_WaitingWidget(new WaitingWidget(this))
    , mp_DeviceWidget(new DeviceWidget(this))
    , mp_ThreadPool(new ThreadPool(this))
{
    // 加载授权框
    PERF_PRINT_BEGIN("SUB_POINT-01", "");
    if (false == DeviceInfoParser::Instance().getRootPassword()) {
        exit(-1);
    }
    END_SUB_POINT("SUB_POINT-01");


    // 初始化窗口相关的内容，比如界面布局，控件大小
    initWindow();

    // 加载设备信息
    refreshDataBase();

    // 关联信号槽
    connect(mp_ThreadPool, &ThreadPool::finished, this, &MainWindow::loadingFinishSlot);
    connect(mp_DeviceWidget, &DeviceWidget::itemClicked, this, &MainWindow::slotListItemClicked);
    connect(mp_DeviceWidget, &DeviceWidget::refreshInfo, this, &MainWindow::slotRefreshInfo);
    connect(mp_DeviceWidget, &DeviceWidget::exportInfo, this, &MainWindow::slotExportInfo);
    connect(this, &MainWindow::fontChange, this, &MainWindow::changeUI);
}

MainWindow::~MainWindow()
{
    // 释放指针
    DELETE_PTR(mp_WaitingWidget);
    DELETE_PTR(mp_DeviceWidget);
    DELETE_PTR(mp_MainStackWidget);
    DELETE_PTR(mp_ThreadPool);
}

void MainWindow::refresh()
{
    // 正在刷新,避免重复操作
    if (m_refreshing) {
        return;
    }

    // 授权框
    if (false == DeviceInfoParser::Instance().getRootPassword()) {
        return;
    }

    // 正在刷新标志
    m_refreshing = true;

    mp_WaitingWidget->start();
    mp_MainStackWidget->setCurrentIndex(0);

    // 加载设备信息
    refreshDataBase();

}

bool MainWindow::exportTo()
{
    QString selectFilter;

    // 导出信息文件保存路径
    static QString saveDir = []() {
        QString dirStr = "./";
        QDir dir(QDir::homePath() + "/Desktop/");
        if (dir.exists()) {
            dirStr = QDir::homePath() + "/Desktop/";
        }
        return dirStr;
    }
    ();

    // 导出信息文件名称
    QString file = DFileDialog::getSaveFileName(
                       this,
                       "Export", saveDir + tr("Device Info", "export file's name") + \
                       QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") .remove(QRegExp("\\s")) + ".txt", \
                       "Text (*.txt);; Doc (*.docx);; Xls (*.xls);; Html (*.html)", &selectFilter);  //

    if (file.isEmpty() == true) {
        return true;
    }

    QFileInfo fileInfo(file);
    //saveDir = fileInfo.absolutePath() + "/";

    // 文件类型txt
    if (selectFilter == "Text (*.txt)") {
        return DeviceManager::instance()->exportToTxt(file);

    }

    // 文件类型html
    if (selectFilter == "Html (*.html)") {
        return DeviceManager::instance()->exportToHtml(file);
    }

    // 文件类型docx
    if (selectFilter == "Doc (*.docx)") {
        return DeviceManager::instance()->exportToDoc(file);
    }

    // 文件类型xls
    if (selectFilter == "Xls (*.xls)") {
        return DeviceManager::instance()->exportToXlsx(file);
    }

    return false;
}



void MainWindow::showDisplayShortcutsHelpDialog()
{
    QJsonDocument doc;

    //获取快捷键json文本
    getJsonDoc(doc);

    // 快捷键窗口位置
    QRect rect = this->window()->geometry();
    QPoint pos(rect.x() + rect.width() / 2,
               rect.y() + rect.height() / 2);

    // 快捷键窗口显示进程
    QProcess *shortcutViewProcess = new QProcess();
    QStringList shortcutString;
    QString param1 = "-j=" + QString(doc.toJson().data());
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    // 启动子进程
    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
}

void MainWindow::addJsonArrayItem(QJsonArray &windowJsonItems, const QString &name, const QString &value)
{
    // 添加json数组对
    QJsonObject jsonObject;
    jsonObject.insert("name", name);
    jsonObject.insert("value", value);
    windowJsonItems.append(jsonObject);
}

void MainWindow::getJsonDoc(QJsonDocument &doc)
{
    QJsonArray jsonGroups;

    // 窗口快捷键组
    QJsonArray windowJsonItems;

    addJsonArrayItem(windowJsonItems, tr("Display shortcuts"), "Ctrl+Shift+?");
    addJsonArrayItem(windowJsonItems, tr("Close"), "Alt+F4");
    addJsonArrayItem(windowJsonItems, tr("Help"), "F1");
    addJsonArrayItem(windowJsonItems, tr("Copy"), "Ctrl+C");

    // 窗口快捷键添加到 系统分类
    QJsonObject windowJsonGroup;
    windowJsonGroup.insert("groupName", tr("System"));
    windowJsonGroup.insert("groupItems", windowJsonItems);
    jsonGroups.append(windowJsonGroup);

    // 编辑快捷键组
    QJsonArray editorJsonItems;

    addJsonArrayItem(editorJsonItems, tr("Export"), "Ctrl+E");
    addJsonArrayItem(editorJsonItems, tr("Refresh"), "F5");

    // 编辑快捷键添加到 设备管理器分类
    QJsonObject editorJsonGroup;
    editorJsonGroup.insert("groupName", tr("Device Manager"));
    editorJsonGroup.insert("groupItems", editorJsonItems);
    jsonGroups.append(editorJsonGroup);

    // 添加快捷键组到对象
    QJsonObject shortcutObj;
    shortcutObj.insert("shortcut", jsonGroups);

    doc.setObject(shortcutObj);
}

void MainWindow::windowMaximizing()
{
    if (isMaximized()) {
        // 正常窗口大小
        showNormal();
    }  else {
        // 窗口最大化
        showMaximized();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    DMainWindow::resizeEvent(event);
}

void MainWindow::initWindow()
{
    //1. 第一步初始化窗口大小
    initWindowSize();

    //2. 初始化界面布局
    initWidgets();
}

void MainWindow::initWindowSize()
{
    // 设置窗口的最小尺寸
    QSize minSize(MIN_WIDTH, MIN_HEIGHT);
    setMinimumSize(minSize);

    // 设置窗口的大小
    QSize initSize(INIT_WIDTH, INIT_HEIGHT);
    resize(initSize);
}

void MainWindow::initWidgets()
{
    // 设置窗口的主控件
    setCentralWidget(mp_MainStackWidget);
    setContentsMargins(0, 0, 0, 0);

    // 添加加载等待界面
    mp_MainStackWidget->addWidget(mp_WaitingWidget);
    mp_WaitingWidget->start();

    // 添加信息显示界面
    mp_MainStackWidget->addWidget(mp_DeviceWidget);
}

void MainWindow::refreshDataBase()
{
    // 设置应用程序强制光标为cursor
    DApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // 启动线程加载设备信息
    if (mp_ThreadPool) {
        mp_ThreadPool->loadCmdInfo();
    }
}

void MainWindow::loadingFinishSlot(const QString &message)
{
    static bool begin = true;
    static qint64 b = QDateTime::currentDateTime().toMSecsSinceEpoch();
    static qint64 c = QDateTime::currentDateTime().toMSecsSinceEpoch();

    if (begin) {
        b = QDateTime::currentDateTime().toMSecsSinceEpoch();
        begin = false;
    }

    // finish 表示所有设备信息加载完成
    if (message == "finish") {
        c = QDateTime::currentDateTime().toMSecsSinceEpoch();
        begin = true;
        qDebug() << "************************&&*************************" << (c - b) ;

        // 一定要有否则指针一直显示圆圈与setOverrideCursor成对使用
        DApplication::restoreOverrideCursor();

        // 信息显示界面
        // 获取设备类型列表
        DeviceManager::instance()->setDeviceListClass();
        const QList<QPair<QString, QString>> types = DeviceManager::instance()->getDeviceTypes();

        // 获取设备驱动列表
        DeviceManager::instance()->getDeviceDriverPool();

        // 更新左侧ListView
        mp_DeviceWidget->updateListView(types);

        // 设置当前页面设备信息页
        mp_MainStackWidget->setCurrentWidget(mp_DeviceWidget);

        // 刷新结束
        m_refreshing = false;

        PERF_PRINT_END_SUB("POINT-01", "SUB_POINT-01");
    }
}

void MainWindow::slotListItemClicked(const QString &itemStr)
{
    QList<DeviceBaseInfo *> lst;
    bool ret = DeviceManager::instance()->getDeviceList(itemStr, lst);
    if (ret) {
        mp_DeviceWidget->updateDevice(itemStr, lst);
    } else {
        QMap<QString, QString> overviewMap = DeviceManager::instance()->getDeviceOverview();
        mp_DeviceWidget->updateOverview(itemStr, overviewMap);
    }
}

void MainWindow::slotRefreshInfo()
{
    // 界面刷新
    refresh();
}

void MainWindow::slotExportInfo()
{
    // 设备信息导出
    exportTo();
}

void MainWindow::changeUI()
{
    // 更新当前设备界面设备
    slotListItemClicked(mp_DeviceWidget->currentIndex());
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    // ctrl+e:导出
    if (e->key() == Qt::Key_E) {
        Qt::KeyboardModifiers modifiers = e->modifiers();
        if (modifiers != Qt::NoModifier) {
            if (modifiers.testFlag(Qt::ControlModifier)) {
                exportTo();
                return;
            }
        }
    }

    // F5:界面刷新
    if (e->key() == Qt::Key_F5) {
        refresh();
        return;
    }

    // ctrl+shift+command:快捷键提示界面
    if (e->key() == Qt::Key_Question) {
        Qt::KeyboardModifiers modifiers = e->modifiers();
        if (modifiers != Qt::NoModifier) {
            if (modifiers.testFlag(Qt::ControlModifier)) {
                showDisplayShortcutsHelpDialog();
                return;
            }
        }
    }

    // ctrl+alt：窗口最大化
    if (e->key() == Qt::Key_F) {
        Qt::KeyboardModifiers modifiers = e->modifiers();
        if (modifiers != Qt::NoModifier) {
            if (modifiers.testFlag(Qt::ControlModifier) && modifiers.testFlag(Qt::AltModifier)) {
                windowMaximizing();
                return;
            }
        }
    }

    return DMainWindow::keyPressEvent(e);
}

bool MainWindow::event(QEvent *event)
{
    // 字体大小改变
    if (QEvent::ApplicationFontChange == event->type()) {
        emit fontChange();
        DWidget::event(event);
    }

    return DMainWindow::event(event);
}
