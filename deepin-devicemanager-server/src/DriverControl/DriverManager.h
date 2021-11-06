/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     kongyunzhen <kongyunzhen@uniontech.com>
*
* Maintainer: kongyunzhen <kongyunzhen@uniontech.com>
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
#ifndef DRIVERMANAGER_H
#define DRIVERMANAGER_H

#include <QObject>

class ModCore;
class DebInstaller;
class QThread;

class DriverManager : public QObject
{
    Q_OBJECT
public:
    explicit DriverManager(QObject *parent = nullptr);
    ~ DriverManager();

    bool unInstallDriver(const QString &moduleName); //驱动卸载
    bool installDriver(const QString &filepath);  // 驱动安装
    //获取依赖当前模块在使用的模块
    QStringList checkModuleInUsed(const QString &modName);
    //检查当前模块是否在黑名单
    bool isBlackListed(const QString &modName);
    //判断文件是否驱动包
    bool isDriverPackage(const QString &filepath);

private:
    void initConnections();
    bool unInstallModule(const QString &moduleName);

signals:
    void sigProgressDetail(int progress, const QString &strDeatils);
    void sigFinished(bool bsuccess);
    void sigDebInstall(const QString &package);
    void sigDebUnstall(const QString &package);

public slots:

private:
    ModCore *mp_modcore = nullptr;
    DebInstaller *mp_debinstaller = nullptr;
    QThread *mp_deboperatethread = nullptr;
    int m_installprocess = 0;
    QString errmsg;

};

#endif // DRIVERMANAGER_H
