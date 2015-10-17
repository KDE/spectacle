# install the .desktop and rc files in the correct place

install(
    PROGRAMS org.kde.spectacle.desktop
    DESTINATION ${XDG_APPS_INSTALL_DIR}
)

install(
    FILES spectacle.notifyrc
    DESTINATION ${KNOTIFYRC_INSTALL_DIR}
)

install(
    FILES spectacle.khotkeys
    DESTINATION ${DATA_INSTALL_DIR}/khotkeys
)
