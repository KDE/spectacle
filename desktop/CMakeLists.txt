# install the .desktop and rc files in the correct place

install(
    PROGRAMS org.kde.kscreengenie.desktop
    DESTINATION ${XDG_APPS_INSTALL_DIR}
)

install(
	FILES kscreengenie.notifyrc
	DESTINATION  ${KNOTIFYRC_INSTALL_DIR}
)