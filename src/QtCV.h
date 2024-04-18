/* SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <QImage>
#include <opencv2/opencv.hpp>

/**
 * Convenience functions for using OpenCV with Qt APIs.
 */
namespace QtCV
{
static constexpr int INVALID_MAT_TYPE = -1;
static_assert(CV_8U == 0);
static_assert(std::same_as<decltype(CV_8U), int>);

inline constexpr int matType(QPixelFormat::TypeInterpretation typeInterpretation)
{
    switch (typeInterpretation) {
    case QPixelFormat::UnsignedByte:
        return CV_8U;
    case QPixelFormat::UnsignedShort:
        return CV_16U;
    case QPixelFormat::FloatingPoint:
        return CV_32F;
    default:
        return INVALID_MAT_TYPE;
    }
}

inline constexpr int matType(QPixelFormat pixelFormat)
{
    const auto baseType = matType(pixelFormat.typeInterpretation());
    if (baseType == INVALID_MAT_TYPE) {
        return INVALID_MAT_TYPE;
    }
    return CV_MAKETYPE(baseType, pixelFormat.channelCount());
}

// Get a cv::Mat that reuses the data of a QImage.
// Use cv::Mat::clone() if the owner of the data might be destroyed before you're done using it.
// Expects an image with the right format. If the image has an ARGB32 format (premultiplied or not),
// it needs to be converted to BGRA. RGBX8888 and RGBA8888 formats shouldn't need to be converted.
inline cv::Mat qImageToMat(QImage &image)
{
    const auto type = matType(image.pixelFormat());
    if (type == INVALID_MAT_TYPE) {
        return {};
    }
    // Use the constructor with cv::Size as the first arg to avoid type ambiguity in the args.
    return cv::Mat(cv::Size{image.width(), image.height()}, type, image.bits(), image.bytesPerLine());
}

// Same as qImageToMat, but with cv::Ptr (subclass of std::shared_ptr).
inline cv::Ptr<cv::Mat> qImageToMatPtr(QImage &image)
{
    const auto type = matType(image.pixelFormat());
    if (type == INVALID_MAT_TYPE) {
        return nullptr;
    }
    return cv::makePtr<cv::Mat>(cv::Size{image.width(), image.height()}, type, image.bits(), image.bytesPerLine());
}

// For use with filters that require odd kernel dimensions.
template<typename Number>
inline auto sigmaToKSize(Number value)
{
    return cvRound(value + 1) | 1;
}

// Stack blur looks like Gaussian blur, but doesn't become as slow with larger kernel sizes.
// Stack blur is unavailable in OpenCV versions before 4.7.
// We need this because of the FreeBSD 14 CI pipeline. FreeBSD 14 is only on OpenCV 4.6.
inline void stackOrGaussianBlurCompatibility(cv::InputArray &in, cv::OutputArray &out, cv::Size ksize, double sigmaX, double sigmaY = 0, [[maybe_unused]] int borderType = cv::BORDER_DEFAULT)
{
#if CV_MAJOR_VERSION > 4 || (CV_MAJOR_VERSION == 4 && CV_MINOR_VERSION >= 7)
    // Replicate the behavior of cv::GaussianBlur with automatic kernel size.
    const auto gaussianKSizeFactor = (in.depth() == CV_8U ? 3 : 4) * 2;
    if( ksize.width <= 0 && sigmaX > 0 ) {
        ksize.width = sigmaToKSize(sigmaX * gaussianKSizeFactor);
    }
    if( ksize.height <= 0 && sigmaY > 0 ) {
        ksize.height = sigmaToKSize(sigmaY * gaussianKSizeFactor);
    }
    cv::stackBlur(in, out, ksize);
#else
    cv::GaussianBlur(in, out, ksize, sigmaX, sigmaY, borderType);
#endif
}
}



