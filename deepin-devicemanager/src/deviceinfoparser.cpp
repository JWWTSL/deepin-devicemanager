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

#include "deviceinfoparser.h"
#include <QObject>
#include <sys/utsname.h>
#include <iostream>
#include <QFile>
#include <QProcess>
#include <QRegExp>
#include <DLog>
#include <com_deepin_daemon_power.h>
#include "commondefine.h"
#include "deviceattributedefine.h"
#include "DeviceManager/DeviceManager.h"
#include "DeviceManager/DeviceMouse.h"
#include "DeviceManager/DeviceCpu.h"
#include "DeviceManager/DeviceGpu.h"
#include "DeviceManager/DeviceStorage.h"
#include "DeviceManager/DeviceMemory.h"
#include "DeviceManager/DeviceMonitor.h"
#include "DeviceManager/DeviceBios.h"
#include "DeviceManager/DeviceAudio.h"
#include "DeviceManager/DeviceBluetooth.h"
#include "DeviceManager/DeviceNetwork.h"
#include "DeviceManager/DeviceKeyboard.h"
#include "DeviceManager/DeviceImage.h"
#include "DeviceManager/DeviceOthers.h"
#include "DeviceManager/DeviceComputer.h"
#include "DeviceManager/DevicePower.h"
#include "DeviceManager/DeviceCdrom.h"
#include "DeviceManager/DevicePrint.h"
#include <DApplication>
#include <DSysInfo>
//#include "logpasswordauth.h"
#include "DMessageBox"
#include <cups.h>
#include "Logger.h"


DWIDGET_USE_NAMESPACE

using PowerInter = com::deepin::daemon::Power;

DCORE_USE_NAMESPACE
//const QString g_lsblkDbKey = "lsblk";
DeviceInfoParser::DeviceInfoParser(): QObject()
{

}

DeviceInfoParser::~DeviceInfoParser()
{

}


//QString defaultLanguage = getenv("LANGUAGE");
//setenv("LANGUAGE", "en_US", 1);
//setenv("LANGUAGE", defaultLanguage.toStdString().c_str(), 1);

QString DeviceInfoParser::loadGeneratorKey()
{
    // 获取设备信息
    QString key = "";
    QString deviceInfo;
    if (!getDeviceInfo(QString("sudo dmidecode -t 1"), deviceInfo, "dmidecode_1.txt")) {
        return key;
    }

    if (deviceInfo.contains("PGU-WBY0")) {  // pangu
        key = "PanGu";
    } else if (deviceInfo.contains("HUAWEI L410 KLVU-WDU0")) { // klu
        key = "KLU";
    } else if (deviceInfo.contains("HUAWEI PGUV-WBX0")) { // panguv
        key = "PanGuV";
    }
    return key;
}

bool DeviceInfoParser::getDeviceInfo(const QString &command, QString &deviceInfo, const QString &debugFile)
{
    if (!deviceInfo.isEmpty()) {
        return true;
    }
//    qint64 begin = QDateTime::currentMSecsSinceEpoch();
    if (false == executeProcess(command)) {
        return false;
    }
//    qint64 end = QDateTime::currentMSecsSinceEpoch();
//    qDebug() << command << " ******************************* " << (end - begin) / 1000.0;
    deviceInfo = standOutput_;
#ifdef TEST_DATA_FROM_FILE
    QFile inputDeviceFile(DEVICEINFO_PATH + "/" + debugFile);
    if (false == inputDeviceFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    deviceInfo = inputDeviceFile.readAll();
    inputDeviceFile.close();
#endif

    return true;
}

bool DeviceInfoParser::getRootPassword()
{
    bool res = runCmd("id -un");  // file path is fixed. So write cmd direct
    if (res == true && standOutput_.trimmed() == "root") {
        return true;
    }

#ifdef TEST_DATA_FROM_FILE
    return true;
#endif
    if (false == executeProcess("sudo whoami")) {
        return false;
    }

    return true;
}

bool DeviceInfoParser::executeProcess(const QString &cmd)
{
#ifdef TEST_DATA_FROM_FILE
    return true;
#endif
    if (false == cmd.startsWith("sudo")) {
        return runCmd(cmd);
    }

    runCmd("id -un");
    if (standOutput_.trimmed() == "root") {
        return runCmd(cmd);
    }

    QString newCmd = "pkexec deepin-devicemanager-authenticateProxy \"";
    newCmd += cmd;
    newCmd += "\"";
    newCmd.remove("sudo");
    return runCmd(newCmd);
}

bool DeviceInfoParser::runCmd(const QString &proxy)
{
    QString key = "devicemanager";
    QString cmd = proxy;
    QProcess process_;
    int msecs = 10000;
    if (cmd.startsWith("pkexec deepin-devicemanager-authenticateProxy")) {
        cmd = proxy + QString(" ") + key;
        msecs = -1;
    }

    process_.start(cmd);

    bool res = process_.waitForFinished(msecs);
    standOutput_ = process_.readAllStandardOutput();
    int exitCode = process_.exitCode();
    if (cmd.startsWith("pkexec deepin-devicemanager-authenticateProxy") && (exitCode == 127 || exitCode == 126)) {
        dError("Run \'" + cmd + "\' failed: Password Error! " + QString::number(exitCode) + "\n");
        return false;
    }

    if (res == false) {
        dError("Run \'" + cmd + "\' failed\n");
    }

    return res;
}

bool DeviceInfoParser::runCmd(const QStringList &cmdList)
{
    QProcess process_;
    process_.start("/bin/bash", cmdList);
    bool res = process_.waitForFinished(10000);
    standOutput_ = process_.readAllStandardOutput();
    return res;
}
