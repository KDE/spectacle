# install the DBus service and interface files in the correct place

configure_file(org.freedesktop.Screenshot.service.in ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Screenshot.service)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Screenshot.service DESTINATION ${DBUS_SERVICES_INSTALL_DIR})
install(FILES org.freedesktop.Screenshot.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})
