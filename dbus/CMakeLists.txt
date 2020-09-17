# install the DBus service and interface files in the correct place
include(ECMGenerateDBusServiceFile)
include(ECMConfiguredInstall)

# Generate and install dbus-activated systemd service
ecm_install_configured_files(INPUT app-org.kde.spectacle.service.in DESTINATION ${SYSTEMD_USER_UNIT_INSTALL_DIR})

ecm_generate_dbus_service_file(
   NAME org.kde.spectacle
   EXECUTABLE "${CMAKE_INSTALL_PREFIX}/bin/spectacle --dbus"
   SYSTEMD_SERVICE app-org.kde.spectacle.service
   DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

install(FILES org.kde.Spectacle.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
