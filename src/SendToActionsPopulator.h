#ifndef SENDTOACTIONSPOPULATOR_H
#define SENDTOACTIONSPOPULATOR_H

#include <QObject>
#include <QString>
#include <QAction>
#include <QList>
#include <QPair>
#include <QVariant>
#include <QMenu>

#include <KLocalizedString>
#include <KService>
#include <KMimeTypeTrader>

class SendToActionsPopulator : public QObject
{
    Q_OBJECT
    Q_ENUMS(SendToActionType)

    public:

    enum SendToActionType {
        HardcodedAction,
        KServiceAction,
        KipiAction
    };

    explicit SendToActionsPopulator(QObject *parent = 0);
    ~SendToActionsPopulator();

    signals:

    void haveAction(const QIcon, const QString, const QVariant);
    void haveSeperator();
    void allDone();

    public slots:

    void process();

    private:

    void sendHardcodedSendToActions();
    void sendKServiceSendToActions();
#ifdef HAVE_KIPI
    void sendKipiSendToActions();
#endif
};

typedef QPair<SendToActionsPopulator::SendToActionType, QString> ActionData;
Q_DECLARE_METATYPE(ActionData)

#endif // SENDTOACTIONSPOPULATOR_H
