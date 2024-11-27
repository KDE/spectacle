/* SPDX-FileCopyrightText: 2022 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "VideoFormatModel.h"
#include "SpectacleCore.h"

#include <KLocalizedString>

using namespace Qt::StringLiterals;

VideoFormatModel::VideoFormatModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_roleNames[Qt::DisplayRole] = "display"_ba;
    m_roleNames[FormatRole] = "format"_ba;
    m_roleNames[ExtensionRole] = "extension"_ba;

    auto platform = SpectacleCore::instance()->videoPlatform();
    connect(platform, &VideoPlatform::supportedFormatsChanged, this, [this, platform]() {
        setFormats(platform->supportedFormats());
    });
    setFormats(platform->supportedFormats());
}

void VideoFormatModel::setFormats(VideoPlatform::Formats formats)
{
    m_data.clear();
    if (formats.testFlag(VideoPlatform::WebM_VP9)) {
        m_data.append({
            i18nc("@item:inlistbox Container/encoder", "WebM/VP9"),
            VideoPlatform::WebM_VP9,
            VideoPlatform::extensionForFormat(VideoPlatform::WebM_VP9),
        });
    }
    if (formats.testFlag(VideoPlatform::MP4_H264)) {
        m_data.append({
            i18nc("@item:inlistbox Container/encoder", "MP4/H.264"),
            VideoPlatform::MP4_H264,
            VideoPlatform::extensionForFormat(VideoPlatform::MP4_H264),
        });
    }
    if (formats.testFlag(VideoPlatform::WebP)) {
        m_data.append({
            i18nc("@item:inlistbox Container/encoder", "Animated WebP"),
            VideoPlatform::WebP,
            VideoPlatform::extensionForFormat(VideoPlatform::WebP),
        });
    }
    Q_EMIT countChanged();
}

int VideoFormatModel::indexOfFormat(VideoPlatform::Format format) const
{
    int finalIndex = -1;
    for (int i = 0; i < m_data.length(); ++i) {
        if (m_data[i].format == format) {
            finalIndex = i;
            break;
        }
    }
    return finalIndex;
}

QHash<int, QByteArray> VideoFormatModel::roleNames() const
{
    return m_roleNames;
}

QVariant VideoFormatModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    QVariant ret;
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return ret;
    }
    if (role == Qt::DisplayRole) {
        ret = m_data.at(row).label;
    } else if (role == FormatRole) {
        ret = m_data.at(row).format;
    } else if (role == ExtensionRole) {
        ret = m_data.at(row).extension;
    }
    return ret;
}

int VideoFormatModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_data.size();
}

#include "moc_VideoFormatModel.cpp"
