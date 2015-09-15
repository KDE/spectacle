# install the .desktop and rc files in the correct place

install(
    PROGRAMS org.kde.kapture.desktop
    DESTINATION ${XDG_APPS_INSTALL_DIR}
)

install(
    FILES kapture.notifyrc
    DESTINATION ${KNOTIFYRC_INSTALL_DIR}
)
