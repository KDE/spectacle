/*
 *  Copyright (C) 2016 Martin Graesslin <mgraesslin@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
#include "KWinWaylandImageGrabber.h"

#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>

#include <QtConcurrentRun>
#include <QFutureWatcher>
#include <qplatformdefs.h>

#include <errno.h>

KWinWaylandImageGrabber::KWinWaylandImageGrabber(QObject *parent) :
    ImageGrabber(parent)
{
}

KWinWaylandImageGrabber::~KWinWaylandImageGrabber() = default;

bool KWinWaylandImageGrabber::onClickGrabSupported() const
{
    return true;
}

void KWinWaylandImageGrabber::grabFullScreen()
{
    // unsupported
    emit pixmapChanged(QPixmap());
}

void KWinWaylandImageGrabber::grabCurrentScreen()
{
    // unsupported
    emit pixmapChanged(QPixmap());
}

void KWinWaylandImageGrabber::grabActiveWindow()
{
    // unsupported
    emit pixmapChanged(QPixmap());
}

void KWinWaylandImageGrabber::grabRectangularRegion()
{
    // unsupported
    emit pixmapChanged(QPixmap());
}

static int readData(int fd, QByteArray &data)
{
    // implementation based on QtWayland file qwaylanddataoffer.cpp
    char buf[4096];
    int retryCount = 0;
    int n;
    while (true) {
        n = QT_READ(fd, buf, sizeof buf);
        // give user 30 sec to click a window, afterwards considered as error
        if (n == -1 && (errno == EAGAIN) && ++retryCount < 30000) {
            usleep(1000);
        } else {
            break;
        }
    }
    if (n > 0) {
        data.append(buf, n);
        n = readData(fd, data);
    }
    return n;
}

void KWinWaylandImageGrabber::grabWindowUnderCursor()
{
    QDBusInterface interface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Screenshot"), QStringLiteral("org.kde.kwin.Screenshot"));

    int mask = 0;
    if (mCaptureDecorations) {
        mask = 1;
    }
    if (mCapturePointer) {
        mask |= 1 << 1;
    }

    int pipeFds[2];
    if (pipe2(pipeFds, O_CLOEXEC|O_NONBLOCK) != 0) {
        emit imageGrabFailed();
        return;
    }
    const int pipeFd = pipeFds[0];

    auto call = interface.asyncCall(QStringLiteral("interactive"), QVariant::fromValue(QDBusUnixFileDescriptor(pipeFds[1])),  mask);

    auto readImage = [pipeFd] () -> QImage {
        QByteArray content;
        if (readData(pipeFd, content) != 0) {
            close(pipeFd);
            return QImage();
        }
        close(pipeFd);
        QDataStream ds(content);
        QImage image;
        ds >> image;
        return image;
    };
    QFutureWatcher<QImage> *watcher = new QFutureWatcher<QImage>(this);
    QObject::connect(watcher, &QFutureWatcher<QImage>::finished, this,
        [watcher, this] {
            watcher->deleteLater();
            const QImage img = watcher->result();
            emit pixmapChanged(QPixmap::fromImage(img));
        }
    );
    watcher->setFuture(QtConcurrent::run(readImage));

    close(pipeFds[1]);
}

void KWinWaylandImageGrabber::grabTransientWithParent()
{
    // unsupported, perform grab window under cursor
    grabWindowUnderCursor();
}

QPixmap KWinWaylandImageGrabber::blendCursorImage(const QPixmap &pixmap, int x, int y, int width, int height)
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    Q_UNUSED(width)
    Q_UNUSED(height)
    return pixmap;
}
