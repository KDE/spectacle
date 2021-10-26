#include <iostream>

#include <KConfig>
#include <KConfigGroup>
#include <QCoreApplication>
#include <QString>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    KConfig spectaclerc(QStringLiteral("spectaclerc"), KConfig::SimpleConfig);
    bool alwaysRememberRegion = KConfigGroup(&spectaclerc, "General").readEntry("alwaysRememberRegion", false);
    bool rememberLastRectangularRegion = KConfigGroup(&spectaclerc, "General").readEntry("rememberLastRectangularRegion", true);

    if (rememberLastRectangularRegion && !alwaysRememberRegion) {
        // Settings::EnumRememberLastRectangularRegion::UntilSpectacleIsClosed
        KConfigGroup(&spectaclerc, "General").writeEntry("rememberLastRectangularRegion", 2);
    } else if (!rememberLastRectangularRegion && alwaysRememberRegion) {
        // Settings::EnumRememberLastRectangularRegion::Always
        KConfigGroup(&spectaclerc, "General").writeEntry("rememberLastRectangularRegion", 1);
    } else {
        // Settings::EnumRememberLastRectangularRegion::Never
        KConfigGroup(&spectaclerc, "General").writeEntry("rememberLastRectangularRegion", 0);
    }

    KConfigGroup(&spectaclerc, "General").deleteEntry("alwaysRememberRegion");
}
