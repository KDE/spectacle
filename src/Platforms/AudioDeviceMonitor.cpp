/* SPDX-FileCopyrightText: 2026 tomek7667 <git@cyber-man.pl>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "AudioDeviceMonitor.h"
#include "spectacle_debug.h"

#include <QCoreApplication>
#include <QHash>
#include <QSocketNotifier>
#include <QTimer>

#include <pipewire/pipewire.h>
#include <spa/utils/result.h>

#include <cerrno>
#include <chrono>

using namespace Qt::StringLiterals;

struct AudioDeviceMonitorPrivate {
    AudioDeviceMonitor *q = nullptr;

    pw_loop *loop = nullptr;
    pw_context *context = nullptr;
    pw_core *core = nullptr;
    pw_registry *registry = nullptr;
    spa_hook coreListener{};
    spa_hook registryListener{};

    // Keyed by the global id of the node
    QHash<uint32_t, AudioDeviceMonitor::Device> devices;
    // Keyed by descriptionKey(), also remembers devices that went away
    QHash<QString, QString> descriptions;
    bool devicesChangedPending = false;

    static void onCoreError(void *data, uint32_t id, int seq, int res, const char *message);
    static void onRegistryGlobal(void *data, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const spa_dict *props);
    static void onRegistryGlobalRemove(void *data, uint32_t id);
};

static QString descriptionKey(AudioDeviceMonitor::DeviceType type, const QString &nodeName)
{
    return QString::number(type) + u':' + nodeName;
}

// Newer PipeWire versions add members, leave those zeroed
static const pw_core_events s_coreEvents = [] {
    pw_core_events events{};
    events.version = PW_VERSION_CORE_EVENTS;
    events.error = &AudioDeviceMonitorPrivate::onCoreError;
    return events;
}();

static const pw_registry_events s_registryEvents = {
    .version = PW_VERSION_REGISTRY_EVENTS,
    .global = &AudioDeviceMonitorPrivate::onRegistryGlobal,
    .global_remove = &AudioDeviceMonitorPrivate::onRegistryGlobalRemove,
};

void AudioDeviceMonitorPrivate::onCoreError(void *data, uint32_t id, int seq, int res, const char *message)
{
    Q_UNUSED(seq)
    auto d = static_cast<AudioDeviceMonitorPrivate *>(data);
    qCWarning(SPECTACLE_LOG) << "PipeWire error while listing audio devices:" << res << message;
    if (id != PW_ID_CORE || res != -EPIPE) {
        return;
    }
    // The daemon went away, e.g. it was restarted. Don't tear the connection
    // down from within its own callback, and give the daemon a moment to
    // come back before reconnecting.
    QTimer::singleShot(0, d->q, [q = d->q] {
        q->disconnectFromPipeWire();
        if (!q->d->devices.isEmpty()) {
            q->d->devices.clear();
            q->scheduleDevicesChanged();
        }
    });
    QTimer::singleShot(std::chrono::seconds(2), d->q, [q = d->q] {
        if (!q->d->core) {
            q->connectToPipeWire();
        }
    });
}

void AudioDeviceMonitorPrivate::onRegistryGlobal(void *data,
                                                 uint32_t id,
                                                 uint32_t permissions,
                                                 const char *type,
                                                 uint32_t version,
                                                 const spa_dict *props)
{
    Q_UNUSED(permissions)
    Q_UNUSED(version)
    if (!props || qstrcmp(type, PW_TYPE_INTERFACE_Node) != 0) {
        return;
    }
    const char *mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
    const char *nodeName = spa_dict_lookup(props, PW_KEY_NODE_NAME);
    if (!mediaClass || !nodeName || !*nodeName) {
        return;
    }

    AudioDeviceMonitor::DeviceType deviceType;
    if (qstrcmp(mediaClass, "Audio/Sink") == 0) {
        deviceType = AudioDeviceMonitor::Output;
    } else if (qstrcmp(mediaClass, "Audio/Source") == 0 || qstrcmp(mediaClass, "Audio/Source/Virtual") == 0) {
        deviceType = AudioDeviceMonitor::Input;
    } else {
        return;
    }

    const char *description = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
    if (!description || !*description) {
        description = spa_dict_lookup(props, PW_KEY_NODE_NICK);
    }
    if (!description || !*description) {
        description = nodeName;
    }

    auto d = static_cast<AudioDeviceMonitorPrivate *>(data);
    AudioDeviceMonitor::Device device{deviceType, QString::fromUtf8(nodeName), QString::fromUtf8(description)};
    d->descriptions.insert(descriptionKey(device.type, device.nodeName), device.description);
    d->devices.insert(id, device);
    d->q->scheduleDevicesChanged();
}

void AudioDeviceMonitorPrivate::onRegistryGlobalRemove(void *data, uint32_t id)
{
    auto d = static_cast<AudioDeviceMonitorPrivate *>(data);
    if (d->devices.remove(id)) {
        d->q->scheduleDevicesChanged();
    }
}

static AudioDeviceMonitor *s_instance = nullptr;

AudioDeviceMonitor *AudioDeviceMonitor::instance()
{
    if (!s_instance) {
        // Parented to the application so that the PipeWire connection and its
        // socket notifier are torn down before the event loop goes away.
        s_instance = new AudioDeviceMonitor(QCoreApplication::instance());
    }
    return s_instance;
}

AudioDeviceMonitor::AudioDeviceMonitor(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<AudioDeviceMonitorPrivate>())
{
    d->q = this;

    pw_init(nullptr, nullptr);
    d->loop = pw_loop_new(nullptr);
    if (!d->loop) {
        qCWarning(SPECTACLE_LOG) << "Could not create a PipeWire loop, audio devices cannot be listed";
        return;
    }
    pw_loop_enter(d->loop);
    auto notifier = new QSocketNotifier(pw_loop_get_fd(d->loop), QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this] {
        const int result = pw_loop_iterate(d->loop, 0);
        if (result < 0) {
            qCWarning(SPECTACLE_LOG) << "Failed to iterate the PipeWire loop:" << spa_strerror(result);
        }
    });

    d->context = pw_context_new(d->loop, nullptr, 0);
    if (!d->context) {
        qCWarning(SPECTACLE_LOG) << "Could not create a PipeWire context, audio devices cannot be listed";
        return;
    }
    connectToPipeWire();
}

AudioDeviceMonitor::~AudioDeviceMonitor()
{
    disconnectFromPipeWire();
    if (d->context) {
        pw_context_destroy(d->context);
    }
    if (d->loop) {
        pw_loop_leave(d->loop);
        pw_loop_destroy(d->loop);
    }
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool AudioDeviceMonitor::connectToPipeWire()
{
    d->core = pw_context_connect(d->context, nullptr, 0);
    if (!d->core) {
        qCWarning(SPECTACLE_LOG) << "Could not connect to PipeWire, audio devices cannot be listed";
        return false;
    }
    pw_core_add_listener(d->core, &d->coreListener, &s_coreEvents, d.get());
    d->registry = pw_core_get_registry(d->core, PW_VERSION_REGISTRY, 0);
    pw_registry_add_listener(d->registry, &d->registryListener, &s_registryEvents, d.get());
    return true;
}

void AudioDeviceMonitor::disconnectFromPipeWire()
{
    if (d->registry) {
        spa_hook_remove(&d->registryListener);
        pw_proxy_destroy(reinterpret_cast<pw_proxy *>(d->registry));
        d->registry = nullptr;
    }
    if (d->core) {
        spa_hook_remove(&d->coreListener);
        pw_core_disconnect(d->core);
        d->core = nullptr;
    }
}

void AudioDeviceMonitor::scheduleDevicesChanged()
{
    // The registry announces every existing node one by one right after
    // connecting, only notify once for the whole batch.
    if (d->devicesChangedPending) {
        return;
    }
    d->devicesChangedPending = true;
    QTimer::singleShot(0, this, [this] {
        d->devicesChangedPending = false;
        Q_EMIT devicesChanged();
    });
}

QList<AudioDeviceMonitor::Device> AudioDeviceMonitor::devices() const
{
    return d->devices.values();
}

QString AudioDeviceMonitor::description(DeviceType type, const QString &nodeName) const
{
    return d->descriptions.value(descriptionKey(type, nodeName), nodeName);
}

#include "moc_AudioDeviceMonitor.cpp"
