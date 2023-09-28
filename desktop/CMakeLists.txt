# install the .desktop and rc files in the correct place

configure_file(org.kde.spectacle.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.spectacle.desktop)
install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.spectacle.desktop
    DESTINATION ${KDE_INSTALL_APPDIR}
)

install( DIRECTORY DESTINATION "${KDE_INSTALL_FULL_DATAROOTDIR}/kglobalaccel" )
install(
    CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E create_symlink \"${KDE_INSTALL_FULL_APPDIR}/org.kde.spectacle.desktop\" \"\$ENV{DESTDIR}${KDE_INSTALL_FULL_DATAROOTDIR}/kglobalaccel/org.kde.spectacle.desktop\")"
)

install(
    FILES spectacle.notifyrc
    DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR}
)

install(
    FILES org.kde.spectacle.appdata.xml
    DESTINATION ${KDE_INSTALL_METAINFODIR}
)
