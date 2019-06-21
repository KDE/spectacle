# install the .desktop and rc files in the correct place

install(
    PROGRAMS org.kde.spectacle.desktop
    DESTINATION ${XDG_APPS_INSTALL_DIR}
)

install( DIRECTORY DESTINATION "${KDE_INSTALL_FULL_DATAROOTDIR}/kglobalaccel" )
install(
    CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"${KDE_INSTALL_FULL_APPDIR}/org.kde.dolphin.desktop\" \"\$ENV{DESTDIR}${KDE_INSTALL_FULL_DATAROOTDIR}/kglobalaccel/org.kde.dolphin.desktop\")"
)

install(
    FILES spectacle.notifyrc
    DESTINATION ${KNOTIFYRC_INSTALL_DIR}
)

install(
    FILES org.kde.spectacle.appdata.xml
    DESTINATION ${KDE_INSTALL_METAINFODIR}
)

install(
    FILES spectacle_shortcuts.upd
    DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR}
)
add_executable(spectacle-migrate-shortcuts MigrateShortcuts.cpp)
target_link_libraries(spectacle-migrate-shortcuts Qt5::DBus KF5::GlobalAccel KF5::ConfigCore KF5::XmlGui KF5::I18n)
install(
    TARGETS spectacle-migrate-shortcuts
    DESTINATION ${KDE_INSTALL_LIBDIR}/kconf_update_bin
)
