/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ImagePlatformKWin.h"
#include "ExportManager.h"
#include "Geometry.h"
#include "QtCV.h"
#include "DebugUtils.h"
#include "ImageMetaData.h"

#include <KWindowSystem>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QFile>
#include <QFileDevice>
#include <QFuture>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <QtConcurrentRun>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

using namespace Qt::StringLiterals;

static const QString s_screenShotService = u"org.kde.KWin.ScreenShot2"_s;
static const QString s_screenShotObjectPath = u"/org/kde/KWin/ScreenShot2"_s;
static const QString s_screenShotInterface = u"org.kde.KWin.ScreenShot2"_s;

static QVariantMap screenShotFlagsToVardict(ImagePlatformKWin::ScreenShotFlags flags)
{
    QVariantMap options;

    if (flags & ImagePlatformKWin::ScreenShotFlag::IncludeCursor) {
        options.insert(u"include-cursor"_s, true);
    }
    if (flags & ImagePlatformKWin::ScreenShotFlag::IncludeDecoration) {
        options.insert(u"include-decoration"_s, true);
    }

    bool includeShadow = flags & ImagePlatformKWin::ScreenShotFlag::IncludeShadow;
    options.insert(u"include-shadow"_s, includeShadow);

    if (flags & ImagePlatformKWin::ScreenShotFlag::NativeSize) {
        options.insert(u"native-resolution"_s, true);
    }

    return options;
}

QImage combinedImage(const QList<QImage> &images)
{
    if (images.empty()) {
        return {};
    }
    if (images.size() == 1) {
        return images.constFirst();
    }
    QRectF imageRect;
    qreal maxDpr = 0;
    ImageMetaData::SubGeometryList geometryList;
    for (auto &i : images) {
        const auto dpr = i.devicePixelRatio();
        const auto rect = QRectF{ImageMetaData::logicalXY(i), i.deviceIndependentSize()};
        maxDpr = std::max(maxDpr, dpr);
        imageRect |= rect;
        geometryList << ImageMetaData::subGeometryPropertyMap(rect, dpr);
    }
    static const auto finalFormat = QImage::Format_RGBA8888_Premultiplied;
    const bool allSameDpr = std::all_of(images.cbegin(), images.cend(), [maxDpr](const QImage &i){
        return i.devicePixelRatio() == maxDpr;
    });
    if (allSameDpr) {
        QImage finalImage{imageRect.size().toSize() * maxDpr, finalFormat};
        QPainter painter(&finalImage);
        for (auto &image : images) {
            // Explicitly setting the position and size so that you don't need to read
            // QPainter source code to understand how this works.
            painter.drawImage({ImageMetaData::logicalXY(image) * maxDpr, image.size()}, image);
        }
        painter.end();
        // Setting DPR after painting prevents it from affecting the coordinates of the QPainter.
        // During testing, setting final image DPR first and relying on QPainter::drawImage
        // automatic scaling would occasionally use the wrong target position. I have no idea why.
        // It might not even be directly related to QPainter, so keep an eye out for bugs like that.
        finalImage.setDevicePixelRatio(maxDpr);
        ImageMetaData::setSubGeometryList(finalImage, geometryList);
        return finalImage;
    }
    // We ceil to the next integer size up so that integer DPR images are always crisp.
    const auto finalDpr = std::ceil(maxDpr);
    // An RGBA8888 based format is needed for compatibility with OpenCV.
    // If we used an ARGB32 based format, we'd need to swap red and blue.
    // Not sure what to do if we end up having different formats for different screens.
    QImage finalImage{imageRect.size().toSize() * finalDpr, finalFormat};
    finalImage.fill(Qt::transparent);
    auto mainMat = QtCV::qImageToMat(finalImage);
    for (auto &image : images) {
        auto rgbaImage = image.format() == finalImage.format() ? image : image.convertedTo(finalFormat);
        const auto mat = QtCV::qImageToMat(rgbaImage);
        // Region Of Interest to put the image in the main image.
        const auto pos = ImageMetaData::logicalXY(rgbaImage) * finalDpr;
        const auto size = rgbaImage.deviceIndependentSize() * finalDpr;
        // Truncate to ints instead of rounding to prevent ROI from going out of bounds.
        const cv::Rect rect(pos.x(), pos.y(), size.width(), size.height());
        const auto imageDpr = image.devicePixelRatio();
        const bool hasIntDpr = static_cast<int>(imageDpr) == imageDpr;
        const auto interpolation = hasIntDpr ? cv::INTER_AREA : cv::INTER_LANCZOS4;
        // Will just copy if there's no difference in size
        cv::resize(mat, mainMat(rect), rect.size(), 0, 0, interpolation);
    }
    finalImage.setDevicePixelRatio(finalDpr);
    ImageMetaData::setSubGeometryList(finalImage, geometryList);
    return finalImage;
}

static ResultVariant allocateImage(const QVariantMap &metadata)
{
    QString errors;

    bool ok;
    // We use int because QImage takes ints for its size
    const int width = metadata.value(u"width"_s).toInt(&ok);
    if (!ok || width <= 0) {
        errors = i18nc("@info", "Bad width for KWin screenshot: %1", QDebug::toString(metadata.value(u"width"_s)));
    }

    const int height = metadata.value(u"height"_s).toInt(&ok);
    if (!ok || height <= 0) {
        const auto string = i18nc("@info", "Bad height for KWin screenshot: %1", QDebug::toString(metadata.value(u"height"_s)));
        if (!errors.isEmpty()) {
            errors = errors % u"\n"_s % string;
        } else {
            errors = string;
        }
    }

    // We use uint because QImage::Format values are all above 0.
    const uint format = metadata.value(u"format"_s).toUInt(&ok);
    if (!ok || format <= QImage::Format_Invalid || format >= QImage::NImageFormats) {
        const auto string = i18nc("@info", "Bad format for KWin screenshot: %1", QDebug::toString(metadata.value(u"format"_s)));
        if (!errors.isEmpty()) {
            errors = errors % u"\n"_s % string;
        } else {
            errors = string;
        }
    }

    return errors.isEmpty() //
        ? ResultVariant{QImage{width, height, static_cast<QImage::Format>(format)}}
        : ResultVariant{errors};
}

static ResultVariant readImage(int fileDescriptor, const QVariantMap &metadata)
{
    QFile file;
    if (!file.open(fileDescriptor, QFileDevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
        close(fileDescriptor);
        return {i18nc("@info", "Could not open file descriptor for reading KWin screenshot.")};
    }

    ResultVariant result = allocateImage(metadata);
    if (result.index() != ResultVariant::Image) {
        return result;
    }
    QImage &resultImage = std::get<ResultVariant::Image>(result);

    bool ok = false;
    qreal scale = metadata.value(u"scale"_s).toReal(&ok);
    if (ok) {
        // NOTE: KWin X11Output DPR is always 1. This is intentional.
        // https://bugs.kde.org/show_bug.cgi?id=474778
        if (KWindowSystem::isPlatformX11() && scale == 1) {
            scale = qGuiApp->devicePixelRatio();
        }
        resultImage.setDevicePixelRatio(scale);
    }

    const auto windowId = metadata.value(u"windowId"_s).toString();
    // No point in storing the windowId in the image since it means nothing to users
    // and can't be used if the window is closed.
    if (!windowId.isEmpty()) {
        QDBusMessage message = QDBusMessage::createMethodCall(u"org.kde.KWin"_s,
                                                              u"/KWin"_s,
                                                              u"org.kde.KWin"_s,
                                                              u"getWindowInfo"_s);
        message.setArguments({windowId});
        const QDBusReply<QVariantMap> reply = QDBusConnection::sessionBus().call(message);
        if (reply.isValid()) {
            ImageMetaData::setWindowTitle(resultImage, reply.value().value(u"caption"_s).toString());
            auto logicalX = Geometry::mapFromPlatformValue(reply.value().value(u"x"_s).toReal(), scale);
            auto logicalY = Geometry::mapFromPlatformValue(reply.value().value(u"y"_s).toReal(), scale);
            ImageMetaData::setLogicalXY(resultImage, logicalX, logicalY);
        }
    }

    const auto screenId = metadata.value(u"screen"_s).toString();
    if (!screenId.isEmpty()) {
        ImageMetaData::setScreen(resultImage, screenId);
        if (windowId.isEmpty()) {
            const auto screens = qGuiApp->screens();
            for (auto screen : screens) {
                if (screen->name() == screenId) {
                    auto logicalPos = Geometry::mapFromPlatformPoint(screen->geometry().topLeft(), scale);
                    ImageMetaData::setLogicalXY(resultImage, logicalPos.x(), logicalPos.y());
                    break;
                }
            }
        }
    }

    QDataStream stream(&file);
    stream.readRawData(reinterpret_cast<char *>(resultImage.bits()), resultImage.sizeInBytes());

    return result;
}

template<typename... ArgType>
ScreenShotSource2::ScreenShotSource2(const QString &methodName, ArgType... arguments)
{
    // Do not set the O_NONBLOCK flag. Code that reads data from the pipe assumes
    // that read() will block if there is no any data yet.
    int pipeFds[2]{-1, -1};
    if (pipe2(pipeFds, O_CLOEXEC) == -1) {
        const auto errnum = errno;
        QTimer::singleShot(0, this, [this, errnum]{
            Q_EMIT finished({i18nc("@info", "pipe2() failed for KWin screenshot: %1", QString::fromLocal8Bit(strerror(errnum)))});
        });
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(s_screenShotService, s_screenShotObjectPath, s_screenShotInterface, methodName);

    QVariantList dbusArguments{arguments...};
    dbusArguments.append(QVariant::fromValue(QDBusUnixFileDescriptor(pipeFds[1])));
    message.setArguments(dbusArguments);
    // For arguments that aren't common to other methods.
    // These can help differentiate between different method calls.
    QString specificArguments;
    int argIndex = 0;
    auto joinArgs = [&specificArguments, &argIndex](const auto &arg) {
        specificArguments = specificArguments % QString{(argIndex == 0 ? u"%"_s : u", %"_s) % QString::number(argIndex + 1)}.arg(QDebug::toString(arg));
        return ++argIndex;
    };
    (joinArgs(arguments) || ...);
    const auto relevantInfo = u"- Method: %1\n- Method specific arguments: %2"_s.arg(methodName, specificArguments);

    // We don't know how long the user will take with CaptureInteractive.
    // -1 generally gives a 25s timeout. Let's pick 60s for CaptureInteractive
    // and hope it's enough. We also need to avoid a potential situation where
    // Spectacle might wait indefinitely for a reply that never comes.
    const int timeout = methodName != u"CaptureInteractive" ? 4000 : 60000;
    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message, timeout);
    close(pipeFds[1]);
    m_pipeFileDescriptor.giveFileDescriptor(pipeFds[0]);

    QTimer *timeoutTimer = nullptr;
    if (SPECTACLE_LOG().isDebugEnabled()) {
        Log::debug() << message;
        timeoutTimer = new QTimer(this);
        timeoutTimer->setInterval(timeout / 3.0);
        timeoutTimer->setSingleShot(true);
        timeoutTimer->callOnTimeout([] {
            Log::debug() << "It's taking an unusually long amount of time to get a screenshot...";
        });
        timeoutTimer->start();
    }

    auto watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, relevantInfo, timeoutTimer]() {
        watcher->deleteLater();
        const QDBusPendingReply<QVariantMap> reply = *watcher;
        if (timeoutTimer) {
            timeoutTimer->stop();
            timeoutTimer->deleteLater();
        }

        if (reply.isError()) {
            if (reply.error().name() == u"org.kde.KWin.ScreenShot2.Error.Cancelled"_s) {
                // don't show error on user cancellation
                Q_EMIT finished(ResultVariant::canceled());
            } else {
                auto error = i18nc("@info", "KWin screenshot request failed:\n%1", reply.error().message());
                if (!relevantInfo.isEmpty()) {
                    error = error % u"\n"_s % i18nc("@info", "Potentially relevant information:\n%1", relevantInfo);
                }
                Q_EMIT finished({error});
            }
        } else {
            const QVariantMap &metadata = reply;
            const QString type = metadata.value(u"type"_s).toString();
            if (type != "raw"_L1) {
                const QString errorString = i18nc("@info", "Unsupported KWin screenshot type metadata: %1", type);
                Q_EMIT finished({errorString});
                return;
            }

            QFuture<ResultVariant> future = QtConcurrent::run(readImage, m_pipeFileDescriptor.takeFileDescriptor(), metadata);
            future.then([this](const ResultVariant &result) {
                Q_EMIT finished(result);
            });
        }
    });
}

ScreenShotSourceArea2::ScreenShotSourceArea2(const QRect &area, ImagePlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(u"CaptureArea"_s,
                        qint32(area.x()),
                        qint32(area.y()),
                        quint32(area.width()),
                        quint32(area.height()),
                        screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceInteractive2::ScreenShotSourceInteractive2(ImagePlatformKWin::InteractiveKind kind, ImagePlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(u"CaptureInteractive"_s, quint32(kind), screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceScreen2::ScreenShotSourceScreen2(const QScreen *screen, ImagePlatformKWin::ScreenShotFlags flags)
// NOTE: As of Qt 6.4, QScreen::name() is not guaranteed to match the result of any native APIs.
// It should not be used to uniquely identify a screen, but it happens to work on X11 and Wayland.
// KWin's ScreenShot2 DBus API uses QScreen::name() as identifiers for screens.
    : ScreenShotSource2(u"CaptureScreen"_s, screen->name(), screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceActiveWindow2::ScreenShotSourceActiveWindow2(ImagePlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(u"CaptureActiveWindow"_s, screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceActiveScreen2::ScreenShotSourceActiveScreen2(ImagePlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(u"CaptureActiveScreen"_s, screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceWorkspace2::ScreenShotSourceWorkspace2(ImagePlatformKWin::ScreenShotFlags flags)
    : ScreenShotSource2(u"CaptureWorkspace"_s, screenShotFlagsToVardict(flags))
{
}

ScreenShotSourceMeta2::ScreenShotSourceMeta2(const QVector<ScreenShotSource2 *> &sources)
{
    const auto size = sources.size();
    m_results.reserve(size); // reserves memory but does not change size
    for (ScreenShotSource2 *source : sources) {
        source->setParent(this);
        connect(source, &ScreenShotSource2::finished, this, [this, size](const ResultVariant &result) {
            m_results.emplaceBack(result);
            if (m_results.size() == size) {
                Q_EMIT finished(m_results);
            }
        });
    }
}

ImagePlatformKWin::ImagePlatformKWin(QObject *parent)
    : ImagePlatform(parent)
{
    auto message = QDBusMessage::createMethodCall(u"org.kde.KWin.ScreenShot2"_s,
                                                  u"/org/kde/KWin/ScreenShot2"_s,
                                                  u"org.freedesktop.DBus.Properties"_s,
                                                  u"Get"_s);
    message.setArguments({u"org.kde.KWin.ScreenShot2"_s, u"Version"_s});

    const QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        m_apiVersion = reply.arguments().constFirst().value<QDBusVariant>().variant().toUInt();
    }

    updateSupportedGrabModes();
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &ImagePlatformKWin::updateSupportedGrabModes);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &ImagePlatformKWin::updateSupportedGrabModes);
}

ImagePlatform::GrabModes ImagePlatformKWin::supportedGrabModes() const
{
    return m_grabModes;
}

void ImagePlatformKWin::updateSupportedGrabModes()
{
    ImagePlatform::GrabModes grabModes = GrabMode::AllScreens | GrabMode::WindowUnderCursor | GrabMode::PerScreenImageNative;

    if (m_apiVersion >= 2) {
        grabModes |= GrabMode::ActiveWindow;
    }

    if (QGuiApplication::screens().count() > 1) {
        grabModes |= GrabMode::CurrentScreen | GrabMode::AllScreensScaled;
    }

    if (m_grabModes != grabModes) {
        m_grabModes = grabModes;
        Q_EMIT supportedGrabModesChanged();
    }
}

ImagePlatform::ShutterModes ImagePlatformKWin::supportedShutterModes() const
{
    return ShutterMode::Immediate;
}

void ImagePlatformKWin::doGrab(ShutterMode, GrabMode grabMode, bool includePointer, bool includeDecorations, bool includeShadow)
{
    ScreenShotFlags flags = ScreenShotFlag::NativeSize;

    flags.setFlag(ScreenShotFlag::IncludeShadow, includeShadow);

    if (includeDecorations) {
        flags |= ScreenShotFlag::IncludeDecoration;
    }
    if (includePointer) {
        flags |= ScreenShotFlag::IncludeCursor;
    }

    switch (grabMode) {
    case GrabMode::AllScreens:
        takeScreenShotWorkspace(flags);
        break;
    case GrabMode::CurrentScreen:
        if (m_apiVersion >= 2) {
            takeScreenShotActiveScreen(flags);
        } else {
            takeScreenShotInteractive(InteractiveKind::Screen, flags);
        }
        break;
    case GrabMode::ActiveWindow:
        takeScreenShotActiveWindow(flags);
        break;
    case GrabMode::TransientWithParent:
    case GrabMode::WindowUnderCursor:
        takeScreenShotInteractive(InteractiveKind::Window, flags);
        break;
    case GrabMode::AllScreensScaled:
        takeScreenShotWorkspace(flags & ~ScreenShotFlags(ScreenShotFlag::NativeSize));
        break;
    case GrabMode::PerScreenImageNative:
        takeScreenShotCroppable(flags);
        break;
    case GrabMode::NoGrabModes:
        Q_EMIT newScreenshotFailed();
        break;
    }
}

void ImagePlatformKWin::trackSource(ScreenShotSource2 *source)
{
    connect(source, &ScreenShotSource2::finished, this, [this, source](const ResultVariant &result) {
        source->deleteLater();
        const auto index = result.index();
        if (index == ResultVariant::Image) {
            Q_EMIT newScreenshotTaken(std::get<ResultVariant::Image>(result));
        } else if (index == ResultVariant::ErrorString) {
            Q_EMIT newScreenshotFailed(std::get<ResultVariant::ErrorString>(result));
        }
    });
}

template<typename OutputSignal>
void ImagePlatformKWin::trackSource(ScreenShotSourceMeta2 *source, OutputSignal outputSignal)
{
    connect(source, &ScreenShotSourceMeta2::finished, this, [this, source, outputSignal](const QList<ResultVariant> &results) {
        source->deleteLater();
        QList<QImage> images;
        QString errorString;
        for (const auto &result : results) {
            const auto index = result.index();
            if (index == ResultVariant::Image) {
                images.push_back(std::get<ResultVariant::Image>(result));
            } else if (index == ResultVariant::ErrorString) {
                errorString.append(std::get<ResultVariant::ErrorString>(result) + u"\n"_s);
            }
        }

        if (!images.empty()) {
            Q_EMIT (this->*outputSignal)(combinedImage(images));
        }
        if (!errorString.isEmpty()) {
            Q_EMIT newScreenshotFailed(errorString);
        }
    });
}

void ImagePlatformKWin::takeScreenShotArea(const QRect &area, ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceArea2(area, flags));
}

void ImagePlatformKWin::takeScreenShotInteractive(InteractiveKind kind, ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceInteractive2(kind, flags));
}

void ImagePlatformKWin::takeScreenShotActiveWindow(ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceActiveWindow2(flags));
}

void ImagePlatformKWin::takeScreenShotActiveScreen(ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceActiveScreen2(flags));
}

void ImagePlatformKWin::takeScreenShotWorkspace(ScreenShotFlags flags)
{
    const auto &screens = qGuiApp->screens();
    QList<ScreenShotSource2 *> sources;
    sources.reserve(screens.count());
    for (auto screen : screens) {
        sources.emplaceBack(new ScreenShotSourceScreen2(screen, flags));
    }
    trackSource(new ScreenShotSourceMeta2(sources), &ImagePlatform::newScreenshotTaken);
}

void ImagePlatformKWin::takeScreenShotCroppable(ScreenShotFlags flags)
{
    const auto &screens = qGuiApp->screens();
    QList<ScreenShotSource2 *> sources;
    sources.reserve(screens.count());
    for (auto screen : screens) {
        sources.emplaceBack(new ScreenShotSourceScreen2(screen, flags));
    }
    trackSource(new ScreenShotSourceMeta2(sources), &ImagePlatform::newCroppableScreenshotTaken);
}

#include "moc_ImagePlatformKWin.cpp"
