/*
 * Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     AaronZhang <ya.zhang@archermind.com>
 *
 * Maintainer: AaronZhang <ya.zhang@archermind.com>
 * Maintainer: Yaobin <yao.bin@archermind.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QList>
#include <DTableWidget>
#include "DScrollArea"
#include "DMenu"
#include "DLabel"
#include <QFile>
#include "../deviceattributedefine.h"
#include "xlsxdocument.h"
#include "document.h"
#include "DPalette"
#include "DFrame"
#include "../commondefine.h"
#include "DTextBrowser"
#include "TextBrowser.h"
#include <QDomDocument>

class QLabel;
class QVBoxLayout;
class TableWidgetAlwaysFocus;
class QGridLayout;
class QAction;
class ColumnWidget;
class LogTreeView;
class QStandardItem;
class DeviceBaseInfo;

struct TableHeader {
    QString head;
    int length;
};

struct DeviceBase {
    QString title_ = nullptr;
    QList<ArticleStruct>  articles_;

    QFont font_;
    int fontSizeType_;
};

class DeviceInfoWidgetBase;

class DeivceInfoBrower: public Dtk::Widget::DTextBrowser
{
    Q_OBJECT
public:
    explicit DeivceInfoBrower(DeviceInfoWidgetBase *parent = nullptr);
public slots:
    void fillClipboard();
protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

    DeviceInfoWidgetBase *deviceInfoWidget_ = nullptr;

    // QWidget interface
protected:
    /*
    *@author yaobin
    *@date 2020-01-03
    *绘制底部两个圆角
    */
    virtual void paintEvent(QPaintEvent *event) override;
    /*
    *@author yaobin
    *@date 2020-01-14
    *重写ctrl + c 事件
    */
    virtual void keyPressEvent(QKeyEvent *event) override;
};


class DeviceInfoWidgetBase : public Dtk::Widget::DWidget
{
    Q_OBJECT
public:
    explicit DeviceInfoWidgetBase(Dtk::Widget::DWidget *parent = nullptr, const QString &deviceName = "");
    ~DeviceInfoWidgetBase() override;
    void initFont();

    virtual bool getOverViewInfo(ArticleStruct &info);

    virtual void initWidget();

    void initContextMenu();

    void setCentralInfo(const QString &info);

    static void toHtmlString(QDomDocument &doc, const DeviceBase &di);
    static QString toHtmlString(const DeviceBase &di);

    void addInfo(const QString &title, const QList<ArticleStruct> &articles);


    void addSubInfo(const QString &subTitle, const QList<ArticleStruct> &articles);

    void addTable(const QStringList &headers, const QList<QStringList> &contentsList);

    void addDevice(const QString &subTitle, const QList<ArticleStruct> &articles, int deviceNumber, bool showTitle = false);

    void initDownWidget();

    QString getDeviceName();

    static int maxDeviceSize(const QStringList &list1, const QStringList &list2, const QStringList &list3);

    void getContextMenu(Dtk::Widget::DMenu **contextMenu);

    QString joinArticle(QList<ArticleStruct> &articles, const QString &split = " ");


protected:
    void addDeviceAttribute(const QString &name, const QString &value, QList<ArticleStruct> &attributes, bool removeZero = false);
    void addOtherDeviceAttribute(const DeviceBaseInfo &device, QList<ArticleStruct> &attributes, bool removeZero = false);
    void contextMenuEvent(QContextMenuEvent *event) override;
    void showEvent(QShowEvent *event) override;

public slots:
    void OnCurrentItemClicked(const QModelIndex &index);
    bool onExportToFile();
    void changeTheme();

public:
    virtual bool exportToTxt(QFile &txtFile);
    virtual bool exportToDoc(Docx::Document &doc);
    virtual bool exportToXls(QXlsx::Document &xlsFile);
    virtual bool exportToHtml(QFile &htmlFile);

    virtual bool exportToTxt(const QString &txtFile);
    virtual bool exportToDoc(const QString &docFile);
    static void resetXlsRowCount();
    virtual bool exportToXls(const QString &xlsFile);
    virtual bool exportToHtml(const QString &htmlFile);

public:
    //Dtk::Widget::DTableWidget* tableWidget_ = nullptr;
    LogTreeView *tableWidget_ = nullptr;
    DeviceBase *titleInfo_ = nullptr;
    QList<DeviceBase> deviceInfos_;

    QVBoxLayout *vLayout_ = nullptr;

    QFrame *downFrame_ = nullptr;

//    Dtk::Widget::DTextBrowser *htmlBrower_ = nullptr;
    TextBrowser *htmlBrower_ = nullptr;

    Dtk::Widget::DMenu *contextMenu_ = nullptr;

    QAction *refreshAction_ = nullptr;
    QAction *exportAction_ = nullptr;

    QList<int> textCursorList_;

    //Dtk::Widget::DLabel* htmlLabel_;


public:
    static bool isFontInit_;
    static QFont titleFont_;
    static QFont subTitleFont_;
    static QFont infoFont_;
    static QFont labelFont_;
    static QFont tableHeaderFont_;
    static QFont tableContentFont_;
    static QFont centralFont_;

protected:
    ArticleStruct overviewInfo_;

    int verticalScrollBarMaxValue = 0;

    //static int currentXlsRow_;
    //ColumnWidget* selectColumnWidget_ = nullptr;

    static bool isPaletteInit_;
    static Dtk::Gui::DPalette defaultPa_;

    bool firstShow_ = true;

    QList<ArticleStruct> m_articles;
    QSet<QString> m_existArticles;
};
