/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ImagePlatformKWin.h"
#include "Config.h"
#include "ExportManager.h"
#include "Geometry.h"
#include "DebugUtils.h"
#include "ImageMetaData.h"

#include <QColorSpace>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QTimer>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

using namespace Qt::StringLiterals;

static const QString s_screenShotService3 = u"org.kde.KWin.ScreenShot3"_s;
static const QString s_screenShotObjectPath3 = u"/org/kde/KWin/ScreenShot3"_s;
static const QString s_screenShotInterface3 = u"org.kde.KWin.ScreenShot3"_s;

static QVariantMap screenShotFlagsToVardict3(ImagePlatformKWin3::ScreenShotFlags flags)
{
    QVariantMap options;

    if (flags & ImagePlatformKWin3::ScreenShotFlag::IncludeCursor) {
        options.insert(u"include-cursor"_s, true);
    }
    if (flags & ImagePlatformKWin3::ScreenShotFlag::IncludeDecoration) {
        options.insert(u"include-decoration"_s, true);
    }

    bool includeShadow = flags & ImagePlatformKWin3::ScreenShotFlag::IncludeShadow;
    options.insert(u"include-shadow"_s, includeShadow);

    if (flags & ImagePlatformKWin3::ScreenShotFlag::NativeSize) {
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
        QImage finalImage{(imageRect.size() * maxDpr).toSize(), finalFormat};
        finalImage.setColorSpace(QColorSpace::SRgb);
        finalImage.fill(QColorConstants::Transparent);
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
    QImage finalImage{(imageRect.size() * finalDpr).toSize(), finalFormat};
    finalImage.setColorSpace(QColorSpace::SRgb);
    finalImage.fill(Qt::transparent);
    QPainter painter(&finalImage);
    for (auto &image : images) {
        const auto pos = ImageMetaData::logicalXY(image) * finalDpr;
        const auto size = (image.deviceIndependentSize() * finalDpr).toSize();
        const auto imageDpr = image.devicePixelRatio();
        const bool hasIntDpr = static_cast<int>(imageDpr) == imageDpr;
        painter.setRenderHint(QPainter::SmoothPixmapTransform, !hasIntDpr);
        painter.drawImage(QRectF{pos, size}, image);
    }
    painter.end();
    finalImage.setDevicePixelRatio(finalDpr);
    ImageMetaData::setSubGeometryList(finalImage, geometryList);
    return finalImage;
}

static ScreenShotSource3::Result readImage3(int fileDescriptor, const QVariantMap &metadata)
{
    bool ok;
    const int width = metadata.value(u"width"_s).toInt(&ok);
    if (!ok || width <= 0) {
        return std::unexpected(i18nc("@info, %1 is the width", "Bad width for KWin screenshot: %1", QDebug::toString(metadata.value(u"width"_s))));
    }

    const int height = metadata.value(u"height"_s).toInt(&ok);
    if (!ok || height <= 0) {
        return std::unexpected(i18nc("@info, %1 is the height", "Bad height for KWin screenshot: %1", QDebug::toString(metadata.value(u"height"_s))));
    }

    const int stride = metadata.value(u"stride"_s).toInt(&ok);
    if (!ok || stride <= 0) {
        return std::unexpected(i18nc("@info, %1 is the stride", "Bad stride for KWin screenshot: %1", QDebug::toString(metadata.value(u"stride"_s))));
    }

    // We use uint because QImage::Format values are all above 0.
    const uint format = metadata.value(u"format"_s).toUInt(&ok);
    if (!ok || format <= QImage::Format_Invalid || format >= QImage::NImageFormats) {
        return std::unexpected(
            i18nc("@info, %1 is a file format like 'png', 'jpg'", "Bad format for KWin screenshot: %1", QDebug::toString(metadata.value(u"format"_s))));
    }

    void *data = mmap(nullptr, stride * height, PROT_READ, MAP_SHARED, fileDescriptor, 0);
    if (data == MAP_FAILED) {
        return std::unexpected(i18n("mmap() failed: %1", QString::fromLocal8Bit(strerror(errno))));
    }

    struct MemoryMap
    {
        void *data;
        size_t size;
    };

    auto memoryMap = new MemoryMap {
        .data = data,
        .size = size_t(height * stride),
    };

    QImage resultImage(static_cast<uchar *>(data), width, height, QImage::Format(format), [](void *data) {
        auto memoryMap = static_cast<MemoryMap *>(data);
        munmap(memoryMap->data, memoryMap->size);
        delete memoryMap;
    }, memoryMap);
    resultImage.setColorSpace(QColorSpace::SRgb);

    qreal scale = metadata.value(u"scale"_s).toReal(&ok);
    if (ok) {
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

    return resultImage;
}

template<typename... ArgType>
ScreenShotSource3::ScreenShotSource3(const QString &methodName, ArgType... arguments)
{
    QDBusMessage message = QDBusMessage::createMethodCall(s_screenShotService3, s_screenShotObjectPath3, s_screenShotInterface3, methodName);

    QVariantList dbusArguments{arguments...};
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
        const QDBusPendingReply<QDBusUnixFileDescriptor, QVariantMap> reply = *watcher;
        if (timeoutTimer) {
            timeoutTimer->stop();
            timeoutTimer->deleteLater();
        }

        if (reply.isError()) {
            if (reply.error().name() == u"org.kde.KWin.ScreenShot3.Error.Cancelled"_s) {
                // don't show error on user cancellation
                Q_EMIT finished(QImage{});
            } else {
                auto error = i18nc("@info, %1 is an error message", "KWin screenshot request failed:\n%1", reply.error().message());
                if (!relevantInfo.isEmpty()) {
                    error = error % u"\n"_s % i18nc("@info", "Potentially relevant information:\n%1", relevantInfo);
                }
                Q_EMIT finished(std::unexpected(error));
            }
        } else {
            const QDBusUnixFileDescriptor image = reply.argumentAt<0>();
            const QVariantMap metadata = reply.argumentAt<1>();

            const QString type = metadata.value(u"type"_s).toString();
            if (type != "raw"_L1) {
                const QString errorString = i18nc("@info", "Unsupported KWin screenshot type metadata: %1", type);
                Q_EMIT finished(std::unexpected(errorString));
                return;
            }

            Q_EMIT finished(readImage3(image.fileDescriptor(), metadata));
        }
    });
}

ScreenShotSourceArea3::ScreenShotSourceArea3(const QRect &area, ImagePlatformKWin3::ScreenShotFlags flags)
    : ScreenShotSource3(u"CaptureArea"_s,
                        qint32(area.x()),
                        qint32(area.y()),
                        quint32(area.width()),
                        quint32(area.height()),
                        screenShotFlagsToVardict3(flags))
{
}

ScreenShotSourceInteractive3::ScreenShotSourceInteractive3(ImagePlatformKWin3::InteractiveKind kind, ImagePlatformKWin3::ScreenShotFlags flags)
    : ScreenShotSource3(u"CaptureInteractive"_s, quint32(kind), screenShotFlagsToVardict3(flags))
{
}

ScreenShotSourceScreen3::ScreenShotSourceScreen3(const QScreen *screen, ImagePlatformKWin3::ScreenShotFlags flags)
    : ScreenShotSource3(u"CaptureScreen"_s, screen->name(), screenShotFlagsToVardict3(flags))
{
}

ScreenShotSourceActiveWindow3::ScreenShotSourceActiveWindow3(ImagePlatformKWin3::ScreenShotFlags flags)
    : ScreenShotSource3(u"CaptureActiveWindow"_s, screenShotFlagsToVardict3(flags))
{
}

ScreenShotSourceActiveScreen3::ScreenShotSourceActiveScreen3(ImagePlatformKWin3::ScreenShotFlags flags)
    : ScreenShotSource3(u"CaptureActiveScreen"_s, screenShotFlagsToVardict3(flags))
{
}

ScreenShotSourceWorkspace3::ScreenShotSourceWorkspace3(ImagePlatformKWin3::ScreenShotFlags flags)
    : ScreenShotSource3(u"CaptureWorkspace"_s, screenShotFlagsToVardict3(flags))
{
}

ScreenShotSourceMeta3::ScreenShotSourceMeta3(const QVector<ScreenShotSource3 *> &sources)
{
    const auto size = sources.size();
    m_results.reserve(size); // reserves memory but does not change size
    for (ScreenShotSource3 *source : sources) {
        source->setParent(this);
        connect(source, &ScreenShotSource3::finished, this, [this, size](const ScreenShotSource3::Result &result) {
            m_results.emplaceBack(result);
            if (m_results.size() == size) {
                Q_EMIT finished(m_results);
            }
        });
    }
}

ImagePlatformKWin3::ImagePlatformKWin3(QObject *parent)
    : ImagePlatform(parent)
{
    updateSupportedGrabModes();
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &ImagePlatformKWin3::updateSupportedGrabModes);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &ImagePlatformKWin3::updateSupportedGrabModes);
}

ImagePlatform::GrabModes ImagePlatformKWin3::supportedGrabModes() const
{
    return m_grabModes;
}

void ImagePlatformKWin3::updateSupportedGrabModes()
{
    ImagePlatform::GrabModes grabModes = GrabMode::AllScreens | GrabMode::WindowUnderCursor | GrabMode::PerScreenImageNative | GrabMode::ActiveWindow;

    if (QGuiApplication::screens().count() > 1) {
        grabModes |= GrabMode::CurrentScreen | GrabMode::AllScreensScaled;
    }

    if (m_grabModes != grabModes) {
        m_grabModes = grabModes;
        Q_EMIT supportedGrabModesChanged();
    }
}

ImagePlatform::ShutterModes ImagePlatformKWin3::supportedShutterModes() const
{
    return ShutterMode::Immediate;
}

void ImagePlatformKWin3::doGrab(ShutterMode, GrabMode grabMode, bool includePointer, bool includeDecorations, bool includeShadow)
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
        takeScreenShotActiveScreen(flags);
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

void ImagePlatformKWin3::trackSource(ScreenShotSource3 *source)
{
    connect(source, &ScreenShotSource3::finished, this, [this, source](const ScreenShotSource3::Result &result) {
        source->deleteLater();
        if (!result.has_value()) {
            Q_EMIT newScreenshotFailed(result.error());
            return;
        }
        if (result.value().isNull()) {
            Q_EMIT newScreenshotCanceled();
            return;
        }
        Q_EMIT newScreenshotTaken(result.value());
    });
}

template<typename OutputSignal>
void ImagePlatformKWin3::trackSource(ScreenShotSourceMeta3 *source, OutputSignal outputSignal)
{
    connect(source, &ScreenShotSourceMeta3::finished, this, [this, source, outputSignal](const QList<ScreenShotSource3::Result> &results) {
        source->deleteLater();
        QList<QImage> images;
        QString errorString;
        for (const auto &result : results) {
            if (result.has_value() && !result.value().isNull()) {
                images.emplace_back(result.value());
            } else {
                errorString.append(result.error() % u'\n');
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

void ImagePlatformKWin3::takeScreenShotArea(const QRect &area, ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceArea3(area, flags));
}

void ImagePlatformKWin3::takeScreenShotInteractive(InteractiveKind kind, ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceInteractive3(kind, flags));
}

void ImagePlatformKWin3::takeScreenShotActiveWindow(ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceActiveWindow3(flags));
}

void ImagePlatformKWin3::takeScreenShotActiveScreen(ScreenShotFlags flags)
{
    trackSource(new ScreenShotSourceActiveScreen3(flags));
}

void ImagePlatformKWin3::takeScreenShotWorkspace(ScreenShotFlags flags)
{
    const auto &screens = qGuiApp->screens();
    QList<ScreenShotSource3 *> sources;
    sources.reserve(screens.count());
    for (auto screen : screens) {
        sources.emplaceBack(new ScreenShotSourceScreen3(screen, flags));
    }
    trackSource(new ScreenShotSourceMeta3(sources), &ImagePlatform::newScreenshotTaken);
}

void ImagePlatformKWin3::takeScreenShotCroppable(ScreenShotFlags flags)
{
    const auto &screens = qGuiApp->screens();
    QList<ScreenShotSource3 *> sources;
    sources.reserve(screens.count());
    for (auto screen : screens) {
        sources.emplaceBack(new ScreenShotSourceScreen3(screen, flags));
    }
    trackSource(new ScreenShotSourceMeta3(sources), &ImagePlatform::newCroppableScreenshotTaken);
}

#include "moc_ImagePlatformKWin.cpp"
