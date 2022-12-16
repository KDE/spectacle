# install the DBus service and interface files in the correct place
include(ECMGenerateDBusServiceFile)
include(ECMConfiguredInstall)

# Generate and install dbus-activated systemd service
ecm_install_configured_files(INPUT app-org.kde.spectacle.service.in DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR})

ecm_generate_dbus_service_file(
   NAME org.kde.Spectacle
   EXECUTABLE "${KDE_INSTALL_FULL_BINDIR}/spectacle --dbus"
   SYSTEMD_SERVICE app-org.kde.spectacle.service
   DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

install(FILES org.kde.Spectacle.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})
