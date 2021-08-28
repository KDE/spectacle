/* This file is part of Spectacle, the KDE screenshot utility
 * SPDX-FileCopyrightText: 2016 Martin Graesslin <mgraesslin@kde.org>

 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlatformKWinWayland.h"

#include <QApplication>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusUnixFileDescriptor>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QImage>
#include <QPixmap>
#include <QScreen>
#include <QtConcurrent>
#include <qplatformdefs.h>

#include <array>

/* -- Static Helpers --------------------------------------------------------------------------- */

static bool readData(int fd, QByteArray &data)
{
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(fd, &readset);
    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    char buf[4096 * 16];

    while (true) {
        int ready = select(FD_SETSIZE, &readset, nullptr, nullptr, &timeout);
        if (ready < 0) {
            qWarning() << "PlatformKWinWayland readData: select() failed" << strerror(errno);
            return false;
        } else if (ready == 0) {
            qWarning("PlatformKWinWayland readData: timeout reading from pipe");
            return false;
        } else {
            int n = read(fd, buf, sizeof buf);

            if (n < 0) {
                qWarning() << "PlatformKWinWayland readData: read() failed" << strerror(errno);
                return false;
            } else if (n == 0) {
                return true;
            } else {
                data.append(buf, n);
            }
        }
    }

    Q_UNREACHABLE();
}

static QImage readImage(int thePipeFd)
{
    QByteArray lContent;
    if (!readData(thePipeFd, lContent)) {
        close(thePipeFd);
        return QImage();
    }
    close(thePipeFd);

    QDataStream lDataStream(lContent);
    QImage lImage;
    lDataStream >> lImage;
    return lImage;
}

static QVector<QImage> readImages(int thePipeFd)
{
    QByteArray lContent;
    if (!readData(thePipeFd, lContent)) {
        close(thePipeFd);
        return QVector<QImage>();
    }
    close(thePipeFd);

    QDataStream lDataStream(lContent);
    lDataStream.setVersion(QDataStream::Qt_DefaultCompiledVersion);

    QImage lImage;
    QVector<QImage> imgs;
    while (!lDataStream.atEnd()) {
        lDataStream >> lImage;
        if (!lImage.isNull()) {
            imgs << lImage;
        }
    }

    return imgs;
}

/* -- General Plumbing ------------------------------------------------------------------------- */

PlatformKWinWayland::PlatformKWinWayland(QObject *parent)
    : Platform(parent)
{
}

QString PlatformKWinWayland::platformName() const
{
    return QStringLiteral("KWinWayland");
}

static std::array<int, 3> s_plasmaVersion = {-1, -1, -1};

std::array<int, 3> findPlasmaMinorVersion()
{
    if (s_plasmaVersion == std::array<int, 3>{-1, -1, -1}) {
        auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                      QStringLiteral("/MainApplication"),
                                                      QStringLiteral("org.freedesktop.DBus.Properties"),
                                                      QStringLiteral("Get"));

        message.setArguments({QStringLiteral("org.qtproject.Qt.QCoreApplication"), QStringLiteral("applicationVersion")});

        const auto resultMessage = QDBusConnection::sessionBus().call(message);
        if (resultMessage.type() != QDBusMessage::ReplyMessage) {
            qWarning() << "Error querying plasma version" << resultMessage.errorName() << resultMessage.errorMessage();
            return s_plasmaVersion;
        }
        QDBusVariant val = resultMessage.arguments().at(0).value<QDBusVariant>();

        const QString rawVersion = val.variant().value<QString>();
        const QVector<QStringRef> splitted = rawVersion.splitRef(QLatin1Char('.'));
        if (splitted.size() != 3) {
            qWarning() << "error parsing plasma version";
            return s_plasmaVersion;
        }
        bool ok;
        int plasmaMajorVersion = splitted[0].toInt(&ok);
        if (!ok) {
            qWarning() << "error parsing plasma major version";
            return s_plasmaVersion;
        }
        int plasmaMinorVersion = splitted[1].toInt(&ok);
        if (!ok) {
            qWarning() << "error parsing plasma minor version";
            return s_plasmaVersion;
        }
        int plasmaPatchVersion = splitted[2].toInt(&ok);
        if (!ok) {
            qWarning() << "error parsing plasma patch version";
            return s_plasmaVersion;
        }
        s_plasmaVersion = {plasmaMajorVersion, plasmaMinorVersion, plasmaPatchVersion};
    }
    return s_plasmaVersion;
}

Platform::GrabModes PlatformKWinWayland::supportedGrabModes() const
{
    Platform::GrabModes lSupportedModes({Platform::GrabMode::AllScreens, GrabMode::WindowUnderCursor});
    QList<QScreen *> screens = QApplication::screens();

    // TODO remove sometime after Plasma 5.21 is released
    // We can handle rectangular selection one one screen not scale factor
    // on Plasma < 5.21
    if (screenshotScreensAvailable() || (screens.count() == 1 && screens.first()->devicePixelRatio() == 1)) {
        lSupportedModes |= Platform::GrabMode::PerScreenImageNative;
    }

    // TODO remove sometime after Plasma 5.20 is released
    auto plasmaVersion = findPlasmaMinorVersion();
    if (plasmaVersion.at(0) != -1 && (plasmaVersion.at(0) != 5 || (plasmaVersion.at(1) >= 20))) {
        lSupportedModes |= Platform::GrabMode::AllScreensScaled;
    }

    if (screens.count() > 1) {
        lSupportedModes |= Platform::GrabMode::CurrentScreen;
    }
    return lSupportedModes;
}

bool PlatformKWinWayland::screenshotScreensAvailable() const
{
    // TODO remove sometime after Plasma 5.21 is released
    auto plasmaVersion = findPlasmaMinorVersion();
    // Screenshot screenshotScreens dbus interface requires Plasma 5.21
    if (plasmaVersion.at(0) != -1 && (plasmaVersion.at(0) != 5 || (plasmaVersion.at(1) >= 21 || (plasmaVersion.at(1) == 20 && plasmaVersion.at(2) >= 80)))) {
        return true;
    } else {
        return false;
    }
}

Platform::ShutterModes PlatformKWinWayland::supportedShutterModes() const
{
    // TODO remove sometime after Plasma 5.20 is released
    auto plasmaVersion = findPlasmaMinorVersion();
    if (plasmaVersion.at(0) != -1 && (plasmaVersion.at(0) != 5 || (plasmaVersion.at(1) >= 20))) {
        return {ShutterMode::Immediate};
    } else {
        return {ShutterMode::OnClick};
    }
}

void PlatformKWinWayland::doGrab(ShutterMode /* theShutterMode */, GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations)
{
    switch (theGrabMode) {
    case GrabMode::AllScreens:
        doGrabHelper(QStringLiteral("screenshotFullscreen"), theIncludePointer, true);
        return;
    case GrabMode::AllScreensScaled:
        doGrabHelper(QStringLiteral("screenshotFullscreen"), theIncludePointer, false);
        return;

    case GrabMode::PerScreenImageNative: {
        const QList<QScreen *> screens = QGuiApplication::screens();
        QStringList screenNames;
        screenNames.reserve(screens.count());
        for (const auto screen : screens) {
            screenNames << screen->name();
        }
        if (screenshotScreensAvailable()) {
            doGrabImagesHelper(QStringLiteral("screenshotScreens"), screenNames, theIncludePointer, true);
        } else {
            // TODO remove sometime after Plasma 5.21 is released
            // Use the dbus call screenshotFullscreen to get a single screen screenshot and treat it as a list of images
            doGrabImagesHelper(QStringLiteral("screenshotFullscreen"), theIncludePointer, true);
        }
        return;
    }
    case GrabMode::CurrentScreen: {
        doGrabHelper(QStringLiteral("screenshotScreen"), theIncludePointer);
        return;
    }
    case GrabMode::WindowUnderCursor: {
        int lOpMask = theIncludeDecorations ? 1 : 0;
        if (theIncludePointer) {
            lOpMask |= 1 << 1;
        }
        doGrabHelper(QStringLiteral("interactive"), lOpMask);
        return;
    }
    case GrabMode::InvalidChoice:
    case GrabMode::ActiveWindow:
    case GrabMode::TransientWithParent:
        emit newScreenshotFailed();
        return;
    }
}

/* -- Grab Helpers ----------------------------------------------------------------------------- */

void PlatformKWinWayland::startReadImage(int theReadPipe)
{
    auto lWatcher = new QFutureWatcher<QImage>(this);
    QObject::connect(lWatcher, &QFutureWatcher<QImage>::finished, this, [lWatcher, this]() {
        lWatcher->deleteLater();
        const QImage lImage = lWatcher->result();
        if (lImage.isNull()) {
            Q_EMIT newScreenshotFailed();
        } else {
            Q_EMIT newScreenshotTaken(QPixmap::fromImage(lImage));
        }
    });
    lWatcher->setFuture(QtConcurrent::run(readImage, theReadPipe));
}

void PlatformKWinWayland::startReadImages(int theReadPipe)
{
    auto lWatcher = new QFutureWatcher<QVector<QImage>>(this);
    QObject::connect(lWatcher, &QFutureWatcher<QVector<QImage>>::finished, this, [lWatcher, this]() {
        lWatcher->deleteLater();
        auto result = lWatcher->result();
        if (result.isEmpty()) {
            Q_EMIT newScreenshotFailed();
        } else {
            Q_EMIT newScreensScreenshotTaken(result);
        }
    });
    lWatcher->setFuture(QtConcurrent::run(readImages, theReadPipe));
}

template<typename... ArgType>
void PlatformKWinWayland::callDBus(const QString &theGrabMethod, int theWriteFile, ArgType... arguments)
{
    QDBusInterface lInterface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Screenshot"), QStringLiteral("org.kde.kwin.Screenshot"));
    QDBusPendingCall pcall = lInterface.asyncCall(theGrabMethod, QVariant::fromValue(QDBusUnixFileDescriptor(theWriteFile)), arguments...);
    checkDbusPendingCall(pcall);
}

void PlatformKWinWayland::checkDbusPendingCall(const QDBusPendingCall &pcall)
{
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pcall, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        if (watcher->isError()) {
            const auto error = watcher->error();
            qWarning() << "Error calling KWin DBus interface:" << error.name() << error.message();
            Q_EMIT newScreenshotFailed();
        }
        watcher->deleteLater();
    });
}

template<typename... ArgType>
void PlatformKWinWayland::doGrabHelper(const QString &theGrabMethod, ArgType... arguments)
{
    int lPipeFds[2];
    if (pipe2(lPipeFds, O_CLOEXEC | O_NONBLOCK) != 0) {
        emit newScreenshotFailed();
        return;
    }

    callDBus(theGrabMethod, lPipeFds[1], arguments...);
    startReadImage(lPipeFds[0]);

    close(lPipeFds[1]);
}

template<typename... ArgType>
void PlatformKWinWayland::doGrabImagesHelper(const QString &theGrabMethod, ArgType... arguments)
{
    int lPipeFds[2];
    if (pipe2(lPipeFds, O_CLOEXEC | O_NONBLOCK) != 0) {
        emit newScreenshotFailed();
        return;
    }

    callDBus(theGrabMethod, lPipeFds[1], arguments...);
    startReadImages(lPipeFds[0]);

    close(lPipeFds[1]);
}
