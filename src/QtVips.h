/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QImage>
#include <vips/vips8>

/**
 * Convenience functions for using libvips with Qt APIs.
 */
namespace QtVips
{
inline constexpr int colorBands(QImage::Format format)
{
    switch (format) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_Grayscale8:
    case QImage::Format_Grayscale16:
    case QImage::Format_Alpha8:
        return 1;
    case QImage::Format_RGB444:
    case QImage::Format_RGB555:
    case QImage::Format_RGB666:
    case QImage::Format_RGB888:
    case QImage::Format_BGR888:
        return 3;
    case QImage::Format_RGB32: // This has alpha set to max
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGBX8888: // This has alpha set to max
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
    case QImage::Format_RGBX64: // This has alpha set to max
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied:
    case QImage::Format_RGBX32FPx4: // This has alpha set to max
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        return 4;
    default: // Other formats are unsupported right now
        return 0;
    }
}

inline constexpr VipsBandFormat vipsBandFormat(QImage::Format format)
{
    switch (format) {
    case QImage::Format_Grayscale8:
    case QImage::Format_Alpha8:
    case QImage::Format_RGB888:
    case QImage::Format_BGR888:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied:
        return VipsBandFormat::VIPS_FORMAT_UCHAR; // 8 bits per color band
    case QImage::Format_Grayscale16:
    case QImage::Format_RGBX64:
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied:
        return VipsBandFormat::VIPS_FORMAT_USHORT; // 16 bits per color band
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        return VipsBandFormat::VIPS_FORMAT_FLOAT; // float per color band
    default: // Other formats are unsupported right now
        return VipsBandFormat::VIPS_FORMAT_NOTSET;
    }
}

// Create a VImage C++ class object by reusing the data in the given QImage.
// NOTE: VImage does not own the memory when created this way. This can be dangerous.
inline vips::VImage vImageFromMemory(QImage &qImage)
{
    return vips::VImage::new_from_memory(qImage.bits(),
                                         qImage.sizeInBytes(),
                                         qImage.width(),
                                         qImage.height(),
                                         colorBands(qImage.format()),
                                         vipsBandFormat(qImage.format()));
}

// Create a VImage C++ class object by making a copy of the data in the given QImage.
inline vips::VImage vImageFromMemoryCopy(QImage &qImage)
{
    return vips::VImage::new_from_memory_copy(qImage.bits(),
                                              qImage.sizeInBytes(),
                                              qImage.width(),
                                              qImage.height(),
                                              colorBands(qImage.format()),
                                              vipsBandFormat(qImage.format()));
}

// Write the data from a VipsImage to a QImage.
inline QImage vipsImageWriteToMemory(VipsImage *vipsImage, QImage::Format format = QImage::Format_ARGB32_Premultiplied)
{
    // NOTE: We must use a cleanup function for memory management here.
    // VipsImage does not own data written to memory.
    // QImage does not own data passed in via the constructor.
    size_t writtenSize = 0;
    auto data = static_cast<uchar *>(vips_image_write_to_memory(vipsImage, &writtenSize));
    return QImage(data, vipsImage->Xsize, vipsImage->Ysize, format, std::free, data);
}

// Write the data from a VImage to a QImage.
inline QImage vImageWriteToMemory(const vips::VImage &vImage, QImage::Format format = QImage::Format_ARGB32_Premultiplied)
{
    return vipsImageWriteToMemory(vImage.get_image(), format);
}
}
