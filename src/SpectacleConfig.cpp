#include "SpectacleConfig.h"

#include <QStandardPaths>

SpectacleConfig::SpectacleConfig(QObject *parent) :
    QObject(parent)
{
    mConfig = KSharedConfig::openConfig(QStringLiteral("spectaclerc"));
    mGeneralConfig = KConfigGroup(mConfig, "General");
    mGuiConfig = KConfigGroup(mConfig, "GuiConfig");
}

SpectacleConfig::~SpectacleConfig()
{}

SpectacleConfig* SpectacleConfig::instance()
{
    static SpectacleConfig instance;
    return &instance;
}

// lastSaveAsLocation

QUrl SpectacleConfig::lastSaveAsLocation()
{
    return mGeneralConfig.readEntry(QStringLiteral("lastSaveAsLocation"),
                                    QUrl::fromUserInput(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)));
}

void SpectacleConfig::setLastSaveAsLocation(const QUrl &location)
{
    mGeneralConfig.writeEntry(QStringLiteral("lastSaveAsLocation"), location);
    mGeneralConfig.sync();
}
