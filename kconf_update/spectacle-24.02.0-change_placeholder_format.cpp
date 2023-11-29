/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ConfigUtils.h"
#include <KConfigGroup>
#include <KSharedConfig>
#include <QRegularExpression>

using namespace Qt::StringLiterals;

const ValueMap oldNewMap{
    {u"%Y"_s, u"<yyyy>"_s},
    {u"%y"_s, u"<yy>"_s},
    {u"%M"_s, u"<MM>"_s},
    {u"%n"_s, u"<MMM>"_s},
    {u"%N"_s, u"<MMMM>"_s},
    {u"%D"_s, u"<dd>"_s},
    {u"%H"_s, u"<hh>"_s},
    {u"%m"_s, u"<mm>"_s},
    {u"%S"_s, u"<ss>"_s},
    {u"%t"_s, u"<t>"_s},
    {u"%T"_s, u"<title>"_s},
};

inline QString changedFormat(QString filenameTemplate)
{
    for (auto it = oldNewMap.cbegin(); it != oldNewMap.cend(); ++it) {
        filenameTemplate.replace(it.key(), it.value());
    }

    QRegularExpression sequenceRE(u"%(\\d*)d"_s);
    auto it = sequenceRE.globalMatch(filenameTemplate);
    while (it.hasNext()) {
        auto match = it.next();
        int padding = 0;
        if (!match.captured(1).isEmpty()) {
            padding = match.captured(1).toInt();
        }
        auto newValue = u"<%1>"_s.arg(u"#"_s, padding, u'#');
        filenameTemplate.replace(match.captured(), newValue);
    }

    return filenameTemplate;
}

int main()
{
    const auto fileName = u"spectaclerc"_s;
    if (!continueUpdate(fileName, u"2024-02-28T00:00:00Z"_s)) {
        return 0;
    }

    // We only need to read spectaclerc, so we use SimpleConfig.
    auto spectaclerc = KSharedConfig::openConfig(fileName, KConfig::SimpleConfig);

    auto imageSaveGroup = spectaclerc->group(QStringLiteral("ImageSave"));
    if (!isEntryDefault(imageSaveGroup, "imageFilenameTemplate")) {
        auto value = imageSaveGroup.readEntry("imageFilenameTemplate");
        imageSaveGroup.writeEntry("imageFilenameTemplate", changedFormat(value));
    }

    auto videoSaveGroup = spectaclerc->group(QStringLiteral("VideoSave"));
    if (!isEntryDefault(videoSaveGroup, "videoFilenameTemplate")) {
        auto value = videoSaveGroup.readEntry("videoFilenameTemplate");
        videoSaveGroup.writeEntry("videoFilenameTemplate", changedFormat(value));
    }

    return spectaclerc->sync() ? 0 : 1;
}
