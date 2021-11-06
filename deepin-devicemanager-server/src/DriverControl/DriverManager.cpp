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
#include "DriverManager.h"
#include "Utils.h"
#include "ModCore.h"
#include "DebInstaller.h"

#include <QThread>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

DriverManager::DriverManager(QObject *parent)
    : QObject(parent)
    , mp_modcore(new ModCore(this))
{
    mp_deboperatethread = new QThread(this);
    mp_debinstaller = new DebInstaller;
    mp_debinstaller->moveToThread(mp_deboperatethread);
    initConnections();
}

DriverManager::~DriverManager()
{
    mp_deboperatethread->quit();
    mp_deboperatethread->wait();
}

void DriverManager::initConnections()
{
    connect(mp_debinstaller, &DebInstaller::installFinished, [&](bool bsuccess) {
        if (bsuccess) {
            sigProgressDetail(100, tr("Install success"));
        } else {
            this->sigProgressDetail(m_installprocess, errmsg);
        }
        sigFinished(bsuccess);
    });
    connect(mp_debinstaller, &DebInstaller::progressChanged, [&](int iprocess) {
        m_installprocess = 20 + static_cast<int>(iprocess * 0.8);
        sigProgressDetail(m_installprocess, "");
    });
    connect(mp_debinstaller, &DebInstaller::errorOccurred, [&](QString errmsg) {
        qInfo() << "signal_installFailedReason:" << errmsg;
        this->errmsg = errmsg;
    });
    connect(this, &DriverManager::sigDebInstall, mp_debinstaller, &DebInstaller::installPackage);
    connect(this, &DriverManager::sigDebUnstall, mp_debinstaller, &DebInstaller::installPackage);
}

/**
 * @brief DriverManager::unInstallDriver 卸载驱动
 * @param modulename 驱动模块名
 * @return  true:卸载成功 false:卸载失败
 */
bool DriverManager::unInstallDriver(const QString &modulename)
{
    bool bsuccess = false;
    sigProgressDetail(5, "");
    QString modulePath = mp_modcore->modGetPath(modulename);
    QString modulePackageName = Utils::file2PackageName(modulePath);
    QString kernelRelease = Utils::kernelRelease();
    sigProgressDetail(10, "");
    //当模块包为内核模块包时不允许删除，采用卸载加黑名单禁用方式
    if (modulePackageName.contains(kernelRelease)) {
        sigProgressDetail(20, "");
        bsuccess = unInstallModule(modulename);
        if (bsuccess) {
            sigProgressDetail(100, "");
        }
        sigFinished(bsuccess);

    } else {
        sigProgressDetail(15, "");
        mp_modcore->rmModForce(modulename);
        sigProgressDetail(20, "");
        emit sigDebUnstall(modulePackageName);
    }

    return  bsuccess;
}

bool DriverManager::installDriver(const QString &filepath)
{
    /*1.检查模块是否已存在 存在报错返回，不报错返回
     *2.拷贝、安装（重复目前不处理，提供接口由前端判断，也不指定KMOD_INSERT_FORCE_MODVERSION）
     *3.检测是否在黑名单，如果在黑名单则需要先去除黑名单设置
     *4.设置开机自起
     */
    sigProgressDetail(1, "start");
    if (!QFile::exists(filepath)) {
        sigProgressDetail(5, "file not exist");
        return  false;
    }

    //模块已被加载
    if (mp_modcore->modIsLoaded(filepath)) {
        sigProgressDetail(10, QString("could not insert module %1 :file exist").arg(filepath));
        return  false;
    }
    sigProgressDetail(10, "");
    QFileInfo fileinfo(filepath);
    QMimeDatabase typedb;
    QMimeType filetype = typedb.mimeTypeForFile(fileinfo);
    if (filetype.filterString().contains("deb")) {
        qInfo() << __func__ << "deb install start";
        sigProgressDetail(15, "");
        emit sigDebInstall(filepath);
        sigProgressDetail(20, "");
    } else {
        //已判断文件是否存在所以必然存在文件名
        QString filename = fileinfo.baseName();
        QString installdir = QString("/lib/modules/%1/custom/%2").arg(Utils::kernelRelease()).arg(mp_modcore->modGetName(filepath));
        QDir installDir(installdir);
        //判断安装路径是否已存在，如果不存在先创建安装目录
        if (installDir.exists() ||
                installDir.mkpath(installdir)) {
            sigProgressDetail(30, "");
            //将文件拷贝到安装目录
            QString installpath = installDir.absoluteFilePath(filename);
            if (QFile::copy(filepath, installpath)) {
                sigProgressDetail(40, "");
                //更新依赖
                Utils::updateModDeps();
                sigProgressDetail(50, "");
                QString modname = mp_modcore->modGetName(installpath);
                ModCore::ErrorCode errcode = mp_modcore->modInstall(modname);
                sigProgressDetail(60, "");
                if (ModCore::Success == errcode) {
                    //处理黑名单
                    mp_modcore->rmFromBlackList(modname);
                    sigProgressDetail(70, "");
                    //如果非内建模块设置开机自启动
                    if (!mp_modcore->modIsBuildIn(modname) && !mp_modcore->setModLoadedOnBoot(modname)) {
                        sigFinished(false);
                        return  false;
                    }
                    sigProgressDetail(100, "");
                } else {
                    //失败将文件移除,只删文件不删路径
                    QFile::remove(installpath);
                    sigFinished(false);
                    return  false;
                }
            } else {
                sigFinished(false);
                return  false;
            }
        }
    }

    return  true;
}

/**
 * @brief DriverManager::checkModuleInUsed 获取依赖当前模块在使用的模块
 * @param modName 模块名 sample: hid or hid.ko /xx/xx/hid.ko
 * @return 返回当前依赖模块列表 like ['usbhid', 'hid_generic']
 */
QStringList DriverManager::checkModuleInUsed(const QString &modName)
{
    if (nullptr == mp_modcore)
        return QStringList();

    return  mp_modcore->checkModuleInUsed(modName);
}

/**
 * @brief DriverManager::isBlackListed 模块是否在黑名单中
 * @param modName 模块名 sample:hid or hid.ko /xx/xx/hid.ko
 * @return true 是 false 否
 */
bool DriverManager::isBlackListed(const QString &modName)
{
    return mp_modcore->modIsBuildIn(modName);
}

/**
 * @brief DriverManager::isModFile 判断文件是否未驱动文件
 * @param filePath 文件路径
 * @return true:是 false 否
 */
bool DriverManager::isDriverPackage(const QString &filepath)
{
    bool bdriver = false;
    QMimeDatabase typedb;
    QMimeType filetype = typedb.mimeTypeForFile(filepath);
    if (filetype.filterString().contains("deb")) {
        bdriver = Utils::isDriverPackage(filepath);
    } else {
        bdriver = mp_modcore->isModFile(filepath);
    }

    return  bdriver;
}

/**
 * @brief DriverManager::unInstallModule 驱动卸载
 * @param moduleName 模块名称 like hid or hid.ko
 * @return true: 成功 false: 失败
 */
bool DriverManager::unInstallModule(const QString &moduleName)
{
    bool bsuccess = true;
    sigProgressDetail(40, "");
    if (!mp_modcore->rmModForce(moduleName)) {
        bsuccess = false;
    } else {
        sigProgressDetail(60, "");
        QString shortname = mp_modcore->modGetName(moduleName);
        if (!mp_modcore->addModBlackList(shortname)) {
            bsuccess = false;
        } else {
            //如果非内建模块检测去除自启动设置,不影响结果所以不以该操作结果做最终结果判断
            if (!mp_modcore->modIsBuildIn(shortname)) {
                mp_modcore->rmModLoadedOnBoot(shortname);
            }
        }
    }
    sigProgressDetail(80, "");
    return  bsuccess;
}


