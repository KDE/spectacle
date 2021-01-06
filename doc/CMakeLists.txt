# documentation

KDOCTOOLS_CREATE_HANDBOOK(
    index.docbook
        INSTALL_DESTINATION ${KDE_INSTALL_DOCBUNDLEDIR}/en
        SUBDIR spectacle
)

KDOCTOOLS_CREATE_MANPAGE(
    man-spectacle.1.docbook 1
    INSTALL_DESTINATION
    ${KDE_INSTALL_MANDIR}
)
