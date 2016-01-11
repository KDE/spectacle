#ifndef SPECTACLECONFIG_H
#define SPECTACLECONFIG_H

#include <QObject>
#include <QUrl>

#include <KSharedConfig>
#include <KConfigGroup>

class SpectacleConfig : public QObject
{
    Q_OBJECT

    // singleton-ize the class

    public:

    static SpectacleConfig* instance();

    private:

    explicit SpectacleConfig(QObject *parent = 0);
    virtual ~SpectacleConfig();

    SpectacleConfig(SpectacleConfig const&) = delete;
    void operator= (SpectacleConfig const&) = delete;

    // everything else

    public:

    QUrl lastSaveAsLocation();
    void setLastSaveAsLocation(const QUrl &location);

    private:

    KSharedConfigPtr mConfig;
    KConfigGroup     mGeneralConfig;
    KConfigGroup     mGuiConfig;
};

#endif // SPECTACLECONFIG_H
