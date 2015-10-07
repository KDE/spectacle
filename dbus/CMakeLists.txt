# install the DBus service and interface files in the correct place

configure_file(org.kde.Spectacle.service.in ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Screenshot.service)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Spectacle.service DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
install(FILES org.kde.Spectacle.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
