#include "SendToActionsPopulator.h"

SendToActionsPopulator::SendToActionsPopulator(QObject *parent) : QObject(parent)
{

}

SendToActionsPopulator::~SendToActionsPopulator()
{

}

void SendToActionsPopulator::process()
{
    sendHardcodedSendToActions();
    emit haveSeperator();
    sendKServiceSendToActions();
#ifdef HAVE_KIPI
    emit haveSeperator();
    sendKipiSendToActions();
#endif
    emit allDone();
}

void SendToActionsPopulator::sendHardcodedSendToActions()
{
    const QVariant data_clip = QVariant::fromValue(ActionData(HardcodedAction, "clipboard"));
    const QVariant data_app = QVariant::fromValue(ActionData(HardcodedAction, "application"));

    emit haveAction(QIcon::fromTheme("edit-copy"), i18n("Copy To Clipboard"), data_clip);
    emit haveAction(QIcon(), i18n("Other Application"), data_app);
}

void SendToActionsPopulator::sendKServiceSendToActions()
{
    const KService::List services = KMimeTypeTrader::self()->query("image/png");

    for (auto service: services) {
        QString name = service->name().replace('&', "&&");
        const QVariant data = QVariant::fromValue(ActionData(KServiceAction, service->menuId()));

        emit haveAction(QIcon::fromTheme(service->icon()), name, data);
    }
}

#ifdef HAVE_KIPI
void SendToActionsPopulator::sendKipiSendToActions()
{
    ;;
}
#endif
