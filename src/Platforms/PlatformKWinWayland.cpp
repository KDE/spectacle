/* This file is part of Spectacle, the KDE screenshot utility
 * Copyright (C) 2016 Martin Graesslin <mgraesslin@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlatformKWinWayland.h"

#include <qplatformdefs.h>
#include <QtConcurrent>
#include <QApplication>
#include <QImage>
#include <QPixmap>
#include <QDBusInterface>
#include <QDBusUnixFileDescriptor>
#include <QDBusPendingCall>
#include <QFutureWatcher>

/* -- Static Helpers --------------------------------------------------------------------------- */

static int readData(int theFile, QByteArray &theDataOut)
{
    // implementation based on QtWayland file qwaylanddataoffer.cpp
    char    lBuffer[4096];
    int     lRetryCount = 0;
    ssize_t lBytesRead = 0;
    while (true) {
        lBytesRead = QT_READ(theFile, lBuffer, sizeof lBuffer);
        // give user 30 sec to click a window, afterwards considered as error
        if (lBytesRead == -1 && (errno == EAGAIN) && ++lRetryCount < 30000) {
            usleep(1000);
        } else {
            break;
        }
    }

    if (lBytesRead > 0) {
        theDataOut.append(lBuffer, lBytesRead);
        lBytesRead = readData(theFile, theDataOut);
    }
    return lBytesRead;
}

static QImage readImage(int thePipeFd)
{
    QByteArray lContent;
    if (readData(thePipeFd, lContent) != 0) {
        close(thePipeFd);
        return QImage();
    }
    close(thePipeFd);

    QDataStream lDataStream(lContent);
    QImage lImage;
    lDataStream >> lImage;
    return lImage;
}

/* -- General Plumbing ------------------------------------------------------------------------- */

PlatformKWinWayland::PlatformKWinWayland(QObject *parent) :
    Platform(parent)
{}

QString PlatformKWinWayland::platformName() const
{
    return QStringLiteral("KWinWayland");
}

Platform::GrabModes PlatformKWinWayland::supportedGrabModes() const
{
    Platform::GrabModes lSupportedModes({ GrabMode::AllScreens, GrabMode::WindowUnderCursor });
    if (QApplication::screens().count() > 1) {
        lSupportedModes |= Platform::GrabMode::CurrentScreen;
    }
    return lSupportedModes;
}

Platform::ShutterModes PlatformKWinWayland::supportedShutterModes() const
{
    return { ShutterMode::OnClick };
}

void PlatformKWinWayland::doGrab(ShutterMode theShutterMode, GrabMode theGrabMode, bool theIncludePointer, bool theIncludeDecorations)
{
    if (theShutterMode != ShutterMode::OnClick) {
        emit newScreenshotFailed();
        return;
    }

    switch(theGrabMode) {
    case GrabMode::AllScreens:
        return doGrabHelper(QStringLiteral("screenshotFullscreen"), theIncludePointer);
    case GrabMode::CurrentScreen:
        return doGrabHelper(QStringLiteral("screenshotScreen"), theIncludePointer);
    case GrabMode::WindowUnderCursor: {
        int lOpMask = theIncludeDecorations ? 1 : 0;
        if (theIncludePointer) {
            lOpMask |= 1 << 1;
        }
        return doGrabHelper(QStringLiteral("interactive"), lOpMask);
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
    QObject::connect(lWatcher, &QFutureWatcher<QImage>::finished, this,
        [lWatcher, this] () {
            lWatcher->deleteLater();
            const QImage lImage = lWatcher->result();
            emit newScreenshotTaken(QPixmap::fromImage(lImage));
        }
    );
    lWatcher->setFuture(QtConcurrent::run(readImage, theReadPipe));
}

template <typename ArgType>
void PlatformKWinWayland::callDBus(const QString &theGrabMethod, ArgType theArgument, int theWriteFile)
{
    QDBusInterface lInterface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Screenshot"), QStringLiteral("org.kde.kwin.Screenshot"));
    QDBusPendingCall pcall = lInterface.asyncCall(theGrabMethod, QVariant::fromValue(QDBusUnixFileDescriptor(theWriteFile)), theArgument);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pcall, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                     this, [this](QDBusPendingCallWatcher* watcher) {
        if (watcher->isError()) {
            const auto error = watcher->error();
            qWarning() << "Error calling KWin DBus interface:" << error.name() << error.message();
            newScreenshotFailed();
        }
        watcher->deleteLater();
    });
}

template <typename ArgType>
void PlatformKWinWayland::doGrabHelper(const QString &theGrabMethod, ArgType theArgument)
{
    int lPipeFds[2];
    if (pipe2(lPipeFds, O_CLOEXEC|O_NONBLOCK) != 0) {
        emit newScreenshotFailed();
        return;
    }

    callDBus(theGrabMethod, theArgument, lPipeFds[1]);
    startReadImage(lPipeFds[0]);

    close(lPipeFds[1]);
}
