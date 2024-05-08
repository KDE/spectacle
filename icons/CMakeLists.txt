# default icons for the hicolor theme.
# have ecm install them

ecm_install_icons(
    ICONS
        sc-apps-spectacle.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)

qt_add_resources(spectacle "spectacle_icons"
    PREFIX "/icons"
    FILES
        256-status-media-recording-started.webp
        256-status-media-recording-pulse.webp
        256-status-media-recording.webp
)
