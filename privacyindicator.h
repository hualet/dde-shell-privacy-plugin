// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QVariantMap>
#include <applet.h>

class QTimer;

namespace dock
{

class PrivacyIndicator : public ds::DApplet
{
  Q_OBJECT
  Q_PROPERTY (bool cameraInUse READ cameraInUse NOTIFY cameraInUseChanged)
  Q_PROPERTY (
      bool microphoneInUse READ microphoneInUse NOTIFY microphoneInUseChanged)
  Q_PROPERTY (
      bool locationInUse READ locationInUse NOTIFY locationInUseChanged)
  Q_PROPERTY (
      bool anyDeviceInUse READ anyDeviceInUse NOTIFY anyDeviceInUseChanged)
  Q_PROPERTY (bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

public:
  explicit PrivacyIndicator (QObject *parent = nullptr);
  ~PrivacyIndicator () override;

  bool
  cameraInUse () const
  {
    return m_cameraInUse;
  }
  bool
  microphoneInUse () const
  {
    return m_microphoneInUse;
  }
  bool
  locationInUse () const
  {
    return m_locationInUse;
  }
  bool
  anyDeviceInUse () const
  {
    return m_cameraInUse || m_microphoneInUse || m_locationInUse;
  }
  bool
  visible () const
  {
    return m_visible;
  }

  void setVisible (bool visible);

  Q_INVOKABLE QVariantMap getDeviceStatus ();
  Q_INVOKABLE QString getTooltipText ();

  virtual bool load () override;
  virtual bool init () override;

Q_SIGNALS:
  void cameraInUseChanged (bool inUse);
  void microphoneInUseChanged (bool inUse);
  void locationInUseChanged (bool inUse);
  void anyDeviceInUseChanged (bool inUse);
  void visibleChanged (bool visible);
  void deviceStatusChanged (const QString &device, bool inUse);

private Q_SLOTS:
  void updateDeviceStatus ();

private:
  void setupTimer ();
  bool checkCameraInUse ();
  bool checkMicrophoneInUse ();
  bool checkLocationInUse ();
  bool isDeviceInUse (const QString &devicePath);
  bool isAudioInputInUse ();

private:
  QTimer *m_timer;
  bool m_cameraInUse;
  bool m_microphoneInUse;
  bool m_locationInUse;
  bool m_visible;
};

} // namespace dock
