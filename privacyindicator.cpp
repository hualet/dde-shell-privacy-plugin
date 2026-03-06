// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "privacyindicator.h"

#include <pluginfactory.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>

Q_LOGGING_CATEGORY (logPrivacy, "org.deepin.dde.dock.shell.privacy")

namespace dock
{

PrivacyIndicator::PrivacyIndicator (QObject *parent)
    : ds::DApplet (parent), m_timer (nullptr), m_cameraInUse (false),
      m_microphoneInUse (false), m_locationInUse (false), m_visible (true)
{
  qCDebug (logPrivacy) << "Privacy indicator shell plugin constructed";
}

PrivacyIndicator::~PrivacyIndicator () {}

bool
PrivacyIndicator::load ()
{
  return true;
}

bool
PrivacyIndicator::init ()
{
  qCDebug (logPrivacy) << "Privacy indicator shell plugin init";
  setupTimer ();
  DApplet::init ();
  return true;
}

void
PrivacyIndicator::setupTimer ()
{
  // 初始获取设备状态
  updateDeviceStatus ();

  // 定时刷新设备状态
  m_timer = new QTimer (this);
  connect (m_timer, &QTimer::timeout, this,
           &PrivacyIndicator::updateDeviceStatus);
  m_timer->start (2000); // 每2秒检测一次
}

void
PrivacyIndicator::setVisible (bool visible)
{
  qCDebug (logPrivacy) << "Setting shell plugin visibility - From:"
                       << m_visible << "To:" << visible;

  if (m_visible != visible)
    {
      m_visible = visible;
      Q_EMIT visibleChanged (visible);
    }
}

QVariantMap
PrivacyIndicator::getDeviceStatus ()
{
  QVariantMap status;
  status["camera"] = m_cameraInUse;
  status["microphone"] = m_microphoneInUse;
  status["location"] = m_locationInUse;
  return status;
}

QString
PrivacyIndicator::getTooltipText ()
{
  QStringList devices;
  if (m_cameraInUse)
    devices.append (tr ("Camera"));
  if (m_microphoneInUse)
    devices.append (tr ("Microphone"));
  if (m_locationInUse)
    devices.append (tr ("Location"));

  return devices.isEmpty () ? QString ()
                            : tr ("Privacy: ") + devices.join (", ");
}

bool
PrivacyIndicator::isDeviceInUse (const QString &devicePath)
{
  // 通过检查 /proc/*/fd 来查看设备是否被打开
  QDir procDir ("/proc");
  QStringList entries = procDir.entryList (QDir::Dirs | QDir::NoDotAndDotDot);

  for (const QString &entry : entries)
    {
      bool ok;
      int pid = entry.toInt (&ok);
      if (!ok || pid <= 0)
        continue;

      QString fdPath = QString ("/proc/%1/fd").arg (pid);
      QDir fdDir (fdPath);
      if (!fdDir.exists ())
        continue;

      QStringList fdEntries = fdDir.entryList (QDir::System | QDir::Hidden);
      for (const QString &fd : fdEntries)
        {
          QString linkTarget = QFile::symLinkTarget (fdPath + "/" + fd);
          if (linkTarget == devicePath)
            {
              return true;
            }
        }
    }
  return false;
}

bool
PrivacyIndicator::checkCameraInUse ()
{
  // 检查常见的摄像头设备节点
  QStringList videoDevices;
  QDir devDir ("/dev");
  QStringList entries
      = devDir.entryList (QStringList () << "video*", QDir::System);
  for (const QString &entry : entries)
    {
      videoDevices.append ("/dev/" + entry);
    }

  for (const QString &device : videoDevices)
    {
      if (isDeviceInUse (device))
        {
          return true;
        }
    }
  return false;
}

bool
PrivacyIndicator::isAudioInputInUse ()
{
  // 通过检查 /proc/asound 来查看音频输入是否被占用
  QFile pcmFile ("/proc/asound/pcm");
  if (!pcmFile.open (QIODevice::ReadOnly | QIODevice::Text))
    {
      return false;
    }

  QByteArray content = pcmFile.readAll ();
  pcmFile.close ();

  // 检查是否有录制流打开
  QDir asoundDir ("/proc/asound");
  QStringList entries
      = asoundDir.entryList (QDir::Dirs | QDir::NoDotAndDotDot);

  for (const QString &entry : entries)
    {
      if (entry.startsWith ("card"))
        {
          QString subStatusPath
              = QString ("/proc/asound/%1/pcm0c/sub0/status").arg (entry);
          QFile statusFile (subStatusPath);
          if (statusFile.open (QIODevice::ReadOnly | QIODevice::Text))
            {
              QByteArray status = statusFile.readAll ();
              statusFile.close ();
              // 如果状态不是 "closed" 或为空，说明正在使用
              QString statusStr = QString (status).trimmed ();
              if (!statusStr.isEmpty () && !statusStr.contains ("closed"))
                {
                  return true;
                }
            }
        }
    }
  return false;
}

bool
PrivacyIndicator::checkMicrophoneInUse ()
{
  return isAudioInputInUse ();
}

bool
PrivacyIndicator::checkLocationInUse ()
{
  // 检查位置服务状态
  // 1. 检查 geoclue 服务是否正在运行且有活动客户端
  QProcess process;
  process.start ("systemctl", QStringList () << "--user"
                                             << "is-active"
                                             << "geoclue.service");
  process.waitForFinished (500);
  QString output = QString (process.readAllStandardOutput ()).trimmed ();

  if (output == "active")
    {
      // 进一步检查是否有应用在使用位置服务
      // 检查 geoclue 的客户端列表
      QFile clientsFile ("/var/lib/geoclue/clients");
      if (clientsFile.exists ())
        {
          return true;
        }

      // 或者检查常见的位置服务标志
      QDir geoDir ("/var/lib/geoclue");
      if (geoDir.exists () && !geoDir.entryList (QDir::Files).isEmpty ())
        {
          return true;
        }
    }

  // 检查是否有应用通过 D-Bus 调用位置服务
  // 这是一个简化的检查，实际可能需要更复杂的逻辑
  return false;
}

void
PrivacyIndicator::updateDeviceStatus ()
{
  bool oldCamera = m_cameraInUse;
  bool oldMicrophone = m_microphoneInUse;
  bool oldLocation = m_locationInUse;
  bool oldAnyDeviceInUse = anyDeviceInUse ();

  m_cameraInUse = checkCameraInUse ();
  m_microphoneInUse = checkMicrophoneInUse ();
  m_locationInUse = checkLocationInUse ();

  if (oldCamera != m_cameraInUse)
    {
      qCDebug (logPrivacy) << "Camera status changed:" << m_cameraInUse;
      Q_EMIT cameraInUseChanged (m_cameraInUse);
      Q_EMIT deviceStatusChanged ("camera", m_cameraInUse);
    }
  if (oldMicrophone != m_microphoneInUse)
    {
      qCDebug (logPrivacy) << "Microphone status changed:"
                           << m_microphoneInUse;
      Q_EMIT microphoneInUseChanged (m_microphoneInUse);
      Q_EMIT deviceStatusChanged ("microphone", m_microphoneInUse);
    }
  if (oldLocation != m_locationInUse)
    {
      qCDebug (logPrivacy) << "Location status changed:" << m_locationInUse;
      Q_EMIT locationInUseChanged (m_locationInUse);
      Q_EMIT deviceStatusChanged ("location", m_locationInUse);
    }

  bool newAnyDeviceInUse = anyDeviceInUse ();
  if (oldAnyDeviceInUse != newAnyDeviceInUse)
    {
      Q_EMIT anyDeviceInUseChanged (newAnyDeviceInUse);
    }
}

} // namespace dock

using namespace dock;
D_APPLET_CLASS (PrivacyIndicator)

#include "privacyindicator.moc"
