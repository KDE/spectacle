/* SPDX-FileCopyrightText: 2026 tomek7667 <git@cyber-man.pl>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QList>
#include <QObject>
#include <QString>

#include <memory>

struct AudioDeviceMonitorPrivate;

/**
 * Keeps track of the PipeWire audio devices that can be recorded along with
 * a screen recording.
 *
 * Devices are identified by their node.name, which is what the recorder uses
 * to target them and stays the same across restarts.
 */
class AudioDeviceMonitor : public QObject
{
    Q_OBJECT
public:
    enum DeviceType {
        Output, ///< An Audio/Sink node, recording captures what is played on it
        Input, ///< An Audio/Source node, e.g. a microphone
    };

    struct Device {
        DeviceType type;
        QString nodeName;
        QString description;
    };

    static AudioDeviceMonitor *instance();

    /// The devices that are currently present, in no particular order
    QList<Device> devices() const;

    /**
     * A human readable name for a device, which also works for devices that
     * were present earlier but are gone now. Falls back to the node name.
     */
    QString description(DeviceType type, const QString &nodeName) const;

Q_SIGNALS:
    void devicesChanged();

private:
    explicit AudioDeviceMonitor(QObject *parent = nullptr);
    ~AudioDeviceMonitor() override;

    bool connectToPipeWire();
    void disconnectFromPipeWire();
    void scheduleDevicesChanged();

    std::unique_ptr<AudioDeviceMonitorPrivate> d;
    friend struct AudioDeviceMonitorPrivate;
};
